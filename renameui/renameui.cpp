
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
size_t                                                  dirlist_sel;
size_t                                                  dirlist_scroll;

char                                                    redraw = 1;

struct termios                                          oterm,nterm;

char                                                    path_temp[PATH_MAX+1];

int                                                     screen_rows = 25;
int                                                     screen_cols = 80;

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
            if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) continue;

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

char temp_render[4096];

void draw_info(size_t sel) {
    printf("\x1B[2H");
    printf("\x1B[0;32m");
    printf("\x1B[K");
    fflush(stdout);

    if (sel >= dirlist.size()) return;

    dirlist_entry_t &ent = dirlist[sel];

    temp_render[sizeof(temp_render)-1] = 0;
    strncpy(temp_render,ent.first.c_str(),sizeof(temp_render)-1);
    temp_render[screen_cols-1] = 0;

    printf("%s\n",temp_render);
}

void draw_row(int sy,size_t sel) {
    if (sel == dirlist_sel)
        printf("\x1B[1;33;44m");
    else
        printf("\x1B[0m");

    printf("\x1B[%d;1H",sy);
    printf("\x1B[K");
    fflush(stdout);

    if (sel >= dirlist.size()) return;

    dirlist_entry_t &ent = dirlist[sel];

    int c = 0;
    int col1 = screen_cols - 10;
    const char *s;
    char tmp[64];

    s = ent.first.c_str();
    while (c < col1) {
        if (*s != 0)
            temp_render[c++] = *s++;
        else
            temp_render[c++] = ' ';
    }

    sprintf(tmp,"%c %llu",S_ISDIR(ent.second.st_mode) ? 'D' : 'F',(unsigned long long)ent.second.st_size);

    s = tmp;
    while (c < screen_cols) {
        if (*s != 0)
            temp_render[c++] = *s++;
        else
            temp_render[c++] = ' ';
    }

    temp_render[c] = 0;
    printf("%s\n",temp_render);
}

#define TOPLIST_ROW 3

void draw_dir(void) {
    unsigned int y;

    printf("\x1B[0m");
    printf("\x1B[2J");
    printf("\x1B[H");
    fflush(stdout);

    printf("In: %s\n",cwd.c_str());

    draw_info(dirlist_sel);

    for (y=TOPLIST_ROW;y <= screen_rows;y++)
        draw_row(y, dirlist_scroll+y-TOPLIST_ROW);
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
    nterm.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK);
    tcsetattr(0/*STDIN*/,TCSANOW,&nterm);

    scan_dir();
    dirlist_sel = 0;
    dirlist_scroll = 0;

    do {
        if (redraw) {
            draw_dir();
            redraw = 0;
        }

        in = read_in();
        if (in.empty()) break;
        if (in == "\x1B" || in == "\x1B\x1B") break;

        if (in == "\x1B[A") { /* up arrow */
            if (dirlist_sel > 0) {
                dirlist_sel--;
                if (dirlist_scroll > dirlist_sel) {
                    dirlist_scroll = dirlist_sel;
                    redraw = 1;
                }
                else {
                    draw_info(dirlist_sel);
                    draw_row(dirlist_sel +     TOPLIST_ROW - dirlist_scroll, dirlist_sel    );
                    draw_row(dirlist_sel + 1 + TOPLIST_ROW - dirlist_scroll, dirlist_sel + 1);
                }
            }
        }
        else if (in == "\x1B[B") { /* down arrow */
            if (dirlist.size() != 0) {
                if (dirlist_sel < (dirlist.size() - 1u)) {
                    dirlist_sel++;
                    if ((dirlist_scroll + (screen_rows - TOPLIST_ROW)) < dirlist_sel) {
                        dirlist_scroll = dirlist_sel - (screen_rows - TOPLIST_ROW);
                        redraw = 1;
                    }
                    else {
                        draw_info(dirlist_sel);
                        draw_row(dirlist_sel - 1 + TOPLIST_ROW - dirlist_scroll, dirlist_sel - 1);
                        draw_row(dirlist_sel     + TOPLIST_ROW - dirlist_scroll, dirlist_sel    );
                    }
                }
            }
        }
        else if (in == "\x08" || in == "\x7F") { /* backspace */
            size_t pos = cwd.find_last_of('/');
            if (pos != std::string::npos && pos > 0) {
                cwd = cwd.substr(0,pos);

                scan_dir();
                dirlist_sel = 0;
                dirlist_scroll = 0;

                redraw = 1;
            }
        }
        else if (in == "\x0D" || in == "\x0A") { /* enter */
            if (dirlist_sel < dirlist.size()) {
                dirlist_entry_t &ent = dirlist[dirlist_sel];

                if (S_ISDIR(ent.second.st_mode)) {
                    cwd += "/";
                    cwd += ent.first;

                    scan_dir();
                    dirlist_sel = 0;
                    dirlist_scroll = 0;

                    redraw = 1;
                }
            }
        }
    } while (1);

    printf("\x1B[0m");
    printf("\x1B[99H");
    fflush(stdout);

    tcsetattr(0/*STDIN*/,TCSANOW,&oterm);
    return 0;
}

