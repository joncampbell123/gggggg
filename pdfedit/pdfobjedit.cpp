/* run this at the top of the same directory you built catalog.txt */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

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

void chomp(char *s) {
    char *e = s + strlen(s) - 1;
    while (e >= s && (*e == '\n' || *e == '\r')) *(e--) = 0;
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

class PDFblob : public vector<uint8_t> {
    bool                        modified = false;
};

class PDFmod {
public:
    map<size_t,PDFblob>         mod_xref;
public:
    bool modified(void) const {
        return !mod_xref.empty();
    }
    bool has_mxref(size_t n) {
        return mod_xref.find(n) != mod_xref.end();
    }
    bool load_mxref(istream &is,const PDFxrefentry &xref,size_t n) {
        /* if the mod_xref is already there, the caller wants us to reload.
         * If the caller wanted to only load if not there, it would have
         * called has_mxref() first */
        if (xref.offset < 0) return false;
        if (xref.length < 0) return false;
        if (xref.length > (1*1024*1024*1024)) return false;

        /* clear existing mod */
        auto &mod = mod_xref[n];
        mod.clear();

        if (is.seekg(xref.offset).tellg() != xref.offset) return false;
        mod.resize((size_t)xref.length); /* will throw C++ badalloc if fails */

        assert((&mod[0] + 1) == &mod[1]); /* make sure std::vector works like we expect */

        if (is.read((char*)(&mod[0]),(streamsize)xref.length).gcount() != (streamsize)xref.length) return false;

        return true;
    }
    bool mxref_to_temp(const string &tmp,size_t n) {
        auto i = mod_xref.find(n);
        if (i == mod_xref.end()) return false;

        int fd = open(tmp.c_str(),O_CREAT|O_TRUNC|O_BINARY|O_WRONLY,0600);
        if (fd < 0) return false;
        if (i->second.size() != 0) {
            if (write(fd,&i->second[0],i->second.size()) != i->second.size()) {
                close(fd);
                return false;
            }
        }
        close(fd);
        return true;
    }
    bool temp_to_mxref(const string &tmp,size_t n) {
        auto i = mod_xref.find(n);
        if (i == mod_xref.end()) return false;

        int fd = open(tmp.c_str(),O_BINARY|O_RDONLY);
        if (fd < 0) return false;
        off_t sz = lseek(fd,0,SEEK_END);
        if (sz >= 0 && sz < (1*1024*1024*1024)) {
            i->second.resize((size_t)sz);
            if (sz != 0) {
                if (lseek(fd,0,SEEK_SET) != 0) {
                    close(fd);
                    return false;
                }
                if (read(fd,&i->second[0],sz) != sz) {
                    close(fd);
                    return false;
                }
            }
        }
        else {
            i->second.clear();
        }

        close(fd);
        return true;
    }
};

void garg(string &r,char* &s) {
    r.clear();

    while (*s == ' ' || *s == '\t') s++;

    if (*s == '\'' || *s == '\"') {
        char quote = *s++;

        while (*s != 0 && *s != quote)
            r += *s++;

        if (*s == quote) s++;
    }
    else {
        while (*s != 0 && !(*s == ' ' || *s == '\t'))
            r += *s++;
    }
}

void run_text_editor(const std::string path) {
    char *argv[64];
    int argc=0;

    argv[argc++] = (char*)"/usr/bin/vim";
    argv[argc++] = (char*)"--";
    argv[argc++] = (char*)path.c_str();
    argv[argc  ] = NULL;

    pid_t pid;

    pid = fork();
    if (pid < 0)
        return; // failed

    if (pid == 0) {
        /* child */
        execv(argv[0],argv);
        _exit(1);
    }
    else {
        /* parent */
        while (waitpid(pid,NULL,0) != pid);
    }
}

void less_pdf(const std::string path) {
    char *argv[64];
    int argc=0;

    argv[argc++] = (char*)"/usr/bin/less";
    argv[argc++] = (char*)"--";
    argv[argc++] = (char*)path.c_str();
    argv[argc  ] = NULL;

    pid_t pid;

    pid = fork();
    if (pid < 0)
        return; // failed

    if (pid == 0) {
        /* child */
        execv(argv[0],argv);
        _exit(1);
    }
    else {
        /* parent */
        while (waitpid(pid,NULL,0) != pid);
    }
}

void runEditor(const char *src) {
    string tempname = string(src) + ".modified.pdf";
    string tempedit = string(src) + ".editobj";
    char line[1024];
    ifstream ifs;
    ofstream ofs;
    PDFmod pdfm;
    string ipm;
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

    while (1) {
        printf("pdf> "); fflush(stdout);
        if (fgets(line,sizeof(line),stdin) == NULL) break;
        chomp(line);

        char *s = line;

        garg(ipm,/*&*/s);
        if (ipm.empty()) {
        }
        else if (ipm == "q") {
            break;
        }
        else if (ipm == "h") {
            printf("q       quit            h       help            vo      view orig\n");
            printf("ve      view edited     eo <n>  edit object\n");
        }
        else if (ipm == "vo") {
            less_pdf(src);
        }
        else if (ipm == "ve") {
            less_pdf(tempname);
        }
        else if (ipm == "eo") {
            garg(ipm,/*&*/s);
            if (ipm.empty()) {
                printf("ERR: need object\n");
                continue;
            }

            long n = strtol(ipm.c_str(),NULL,0);
            if (n >= 0 && n < (long)pdf.xref.xreflist.size()) {
                if (!pdfm.has_mxref((size_t)n)) {
                    auto &xref = pdf.xref.xref((size_t)n);
                    if (!pdfm.load_mxref(ifs,xref,(size_t)n)) {
                        printf("ERR: Failed to load xref\n");
                        continue;
                    }

                    printf("INFO: object %ld loaded\n",n);
                }

                if (!pdfm.mxref_to_temp(tempedit,(size_t)n)) {
                    printf("ERR: object %ld failed to send to temp\n",n);
                    continue;
                }

                run_text_editor(tempedit);

                if (!pdfm.temp_to_mxref(tempedit,(size_t)n)) {
                    printf("ERR: object %ld failed to send to temp\n",n);
                    continue;
                }

                if (unlink(tempedit.c_str()) != 0)
                    printf("WARN: unable to remove temp file %s\n",tempedit.c_str());
            }
            else {
                printf("ERR: Out of range\n");
            }
        }
        else {
            printf("ERR: Unknown command '%s'\n",ipm.c_str());
        }
    }

    printf("\n");
}

int main(int argc,char **argv) {
	if (argc < 2) {
		fprintf(stderr,"%s <file>\n",argv[0]);
		return 1;
	}

	runEditor(argv[1]);
	return 0;
}

