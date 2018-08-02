
#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <algorithm>

std::string                                             cwd;

typedef std::pair<std::string,struct stat>              dirlist_entry_t;

std::vector<dirlist_entry_t>                            dirlist;
int                                                     dirlist_sel = -1;

char                                                    redraw = 1;

struct termios                                          oterm,nterm;

char                                                    path_temp[PATH_MAX+1];

bool dirlist_cmpi(const dirlist_entry_t &a,const dirlist_entry_t &b) {
    int d = strcasecmp(a.first.c_str(),b.first.c_str());
    return d < 0;
}

std::string get_cwd(void) {
    char *c = getcwd(path_temp,sizeof(path_temp)-1);
    if (c == NULL) return std::string();
    return c;
}

void scan_dir(void) {
    struct dirent *d;
    struct stat st;
    DIR *dir;

    dirlist.clear();

    if ((dir = opendir(cwd.c_str())) != NULL) {
        while ((d=readdir(dir)) != NULL) {
            dirlist_entry_t ent;

            if (d->d_name[0] == '.') continue;

            if (lstat((cwd + "/" + d->d_name).c_str(), &st) != 0) continue;

            ent.first = d->d_name;
            ent.second = st;

            dirlist.push_back(ent);
        }
        closedir(dir);
    }

    std::sort(dirlist.begin(), dirlist.end(), dirlist_cmpi);
}

char read_char(void) {
    char c = 0;

    read(0/*STDIN*/,&c,1);

    return c;
}

std::string read_in(void) {
    std::string ret;
    char c;

    c = read_char();
    ret += c;

    if (c == 27) {
        c = read_char();
        ret += c;

        if (c == '[') {
            do {
                c = read_char();
                ret += c;
            } while(isdigit(c));
        }
    }

    return ret;
}

int main() {
    std::string in;

    cwd = get_cwd();
    if (cwd.empty()) {
        fprintf(stderr,"Cannot read CWD\n");
        return 1;
    }

    tcgetattr(0/*STDIN*/,&oterm);
    nterm = oterm;
    nterm.c_lflag &= ~ICANON;
    tcsetattr(0/*STDIN*/,TCSANOW,&nterm);

    scan_dir();

    do {
        if (redraw) {
            redraw = 0;
        }

        in = read_in();
        if (in.empty()) break;
        if (in == "\x1B" || in == "\x1B\x1B") break;
    } while (1);

    tcsetattr(0/*STDIN*/,TCSANOW,&oterm);
    return 0;
}

