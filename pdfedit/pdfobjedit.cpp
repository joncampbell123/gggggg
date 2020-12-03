/* run this at the top of the same directory you built catalog.txt */

#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <map>
#include <list>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

#ifndef O_BINARY
#define O_BINARY (0)
#endif

using namespace std;

char tmp[16384];

class PDFxrefentry {
public:
    off_t                       offset = -1;
    off_t                       length = -1;
    uint32_t                    generation = 0;
    char                        use = 0;
public:
    bool readxrefentry(istream &is) {
        char tmp[20];

        offset = -1;
        length = -1;
        generation = 0;
        use = 0;

        if (is.read(tmp,20).gcount() != 20) return false;

        /* the last 2 bytes should be " \r" " \n" or "\r\n" */
        if (memcmp(tmp+18," \r",2) && memcmp(tmp+18," \n",2) && memcmp(tmp+18,"\r\n",2))
            return false;

        if (!isdigit(tmp[0])) return false;
        offset = (off_t)strtoul(tmp,NULL,10);
        if (tmp[10] != ' ') return false;

        if (!isdigit(tmp[11])) return false;
        generation = (uint32_t)strtoul(tmp+11,NULL,10);
        if (tmp[11+5] != ' ') return false;

        use = tmp[17];
        if (use == ' ') return false;

        return true;
    };
};

class PDFxref {
public:
    vector<PDFxrefentry>        xreflist; /* object 0 = elem 0 */
    off_t                       trailer_ofs = -1;
    off_t                       startxref = -1;
public:
    const PDFxrefentry &xref(const size_t i) const {
        if (i >= xreflist.size()) throw runtime_error("xref out of range");
        return xreflist[i];
    }
    void xreflistmksize(void) {
        vector< pair<off_t,size_t> > offsets;

        for (auto i=xreflist.begin();i!=xreflist.end();i++)
            offsets.push_back( pair<off_t,size_t>((*i).offset,(size_t)(i-xreflist.begin())) );

        sort(offsets.begin(),offsets.end());

        {
            off_t poff,coff;
            auto i = offsets.begin();
            if (i != offsets.end()) {
                poff = (*i).first;
                do {
                    auto ci = i; i++;

                    if (i != offsets.end())
                        coff = (*i).first;
                    else
                        coff = startxref;

                    assert(coff >= poff);
                    assert((*ci).second < xreflist.size());

                    auto &xref = xreflist[(*ci).second];
                    assert((*ci).first == xref.offset);
                    xref.length = coff-poff;
                    poff = coff;

#if 0
                    printf("%lu: ofs=%llu len=%llu\n",
                        (unsigned long)(*ci).second,
                        (unsigned long long)xref.offset,
                        (unsigned long long)xref.length);
#endif
                } while (i != offsets.end());
            }
        }
    }
};

void chomp(string &s) {
    size_t l = s.length();
    while (l > 0 && (s[l-1] == '\n' || s[l-1] == '\r')) s.resize(--l);
}

struct PDFxrefrange {
    unsigned long               start = 0;
    unsigned long               count = 0;

    bool parse(const string &line) {
        start = count = 0;

        const char *s = line.c_str();
        /* <start> <count> */
        while (*s == ' ' || *s == '\t') s++;
        if (!isdigit(*s)) return false;
        start = strtoul(s,(char**)(&s),10);
        while (*s == ' ' || *s == '\t') s++;
        if (!isdigit(*s)) return false;
        count = strtoul(s,(char**)(&s),10);
        while (*s == ' ' || *s == '\t') s++;
        if (*s != 0) return false;

        return true;
    }
};

class PDF {
public:
    PDFxref                     xref;
public:
    bool load_xref(istream &is) {
        PDFxrefrange xrr;
        string line;

        if (xref.startxref < 0) return false;
        if (is.seekg(xref.startxref).tellg() != xref.startxref) return false;

        line.clear();
        getline(is,line);
        chomp(line);
        if (line != "xref") return false;

        line.clear();
        getline(is,line);
        chomp(line);
        if (!xrr.parse(line)) return false;

        fprintf(stderr,"Loading objects %lu-%lu\n",xrr.start,xrr.start+xrr.count-1);
        if (xref.xreflist.size() < (xrr.count+xrr.start))
            xref.xreflist.resize(xrr.count+xrr.start);

        for (unsigned long count=0;count < xrr.count;count++) {
            PDFxrefentry ent;

            if (!ent.readxrefentry(is)) return false;
            assert((count+xrr.start) < xref.xreflist.size());
            xref.xreflist[count+xrr.start] = ent;
        }

        return true;
    }
    bool find_startxref(istream &is) {
        off_t sz = (off_t)is.seekg(0,ios_base::end).tellg();
        if (sz < 0) return false;

        char tmp[4096];
        off_t chk = max((off_t)0,sz - (off_t)sizeof(tmp));
        if (is.seekg(chk).tellg() != chk) return false;
        streamsize rds = is.read(tmp,sizeof(tmp)-1).gcount();
        if (rds <= 0) return false;
        assert(rds < sizeof(tmp));
        tmp[rds] = 0;

        char *scan = tmp+rds-1;
        while (scan >= tmp && (*scan == '\t' || *scan == ' ' || *scan == '\n')) *(scan--) = 0;
        while (scan >= tmp && !(*scan == '\n')) scan--;
        if (scan < tmp) return false;
        assert(*scan == '\n');
        scan++;
        if (strcmp(scan,"%%EOF")) return false;
        scan--;
        while (scan >= tmp && (*scan == '\t' || *scan == ' ' || *scan == '\n')) *(scan--) = 0;
        while (scan >= tmp && !(*scan == '\n')) scan--;
        if (scan < tmp) return false;
        assert(*scan == '\n');
        scan++;
        if (!isxdigit(*scan)) return false;
        xref.startxref = strtoul(scan,NULL,10);

        return true;
    }
    bool load(istream &ifs) {
        if (!find_startxref(ifs)) {
            fprintf(stderr,"Cannot locate xref\n");
            return false;
        }
        fprintf(stderr,"PDF: startxref points to file offset %lld\n",(signed long long)xref.startxref);

        if (!load_xref(ifs)) {
            fprintf(stderr,"Cannot load xref\n");
            return false;
        }

        xref.xreflistmksize();

        return true;
    }
};

void runEditor(const char *src) {
    ifstream ifs;
    PDF pdf;

    ifs.open(src,ios_base::in|ios_base::binary);
    if (!ifs.is_open()) {
        fprintf(stderr,"Failed to open %s\n",src);
        return;
    }

    if (!pdf.load(ifs)) {
        fprintf(stderr,"Failed to load %s\n",src);
        return;
    }
}

int main(int argc,char **argv) {
	if (argc < 2) {
		fprintf(stderr,"%s <file>\n",argv[0]);
		return 1;
	}

	runEditor(argv[1]);
	return 0;
}

