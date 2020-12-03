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
};

class PDFxref {
public:
    vector<PDFxrefentry>        xreflist; /* object 0 = elem 0 */
    off_t                       trailer_ofs = -1;
    off_t                       startxref = -1;
};

class PDF {
public:
    PDFxref                     xref;
public:
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
};

void runEditor(const char *src) {
    ifstream ifs;
    PDF pdf;

    ifs.open(src,ios_base::in|ios_base::binary);
    if (!ifs.is_open()) {
        fprintf(stderr,"Failed to open %s\n",src);
        return;
    }

    if (!pdf.find_startxref(ifs)) {
        fprintf(stderr,"Cannot locate xref\n");
        return;
    }
    fprintf(stderr,"PDF: startxref points to file offset %lld\n",(signed long long)pdf.xref.startxref);
}

int main(int argc,char **argv) {
	if (argc < 2) {
		fprintf(stderr,"%s <file>\n",argv[0]);
		return 1;
	}

	runEditor(argv[1]);
	return 0;
}

