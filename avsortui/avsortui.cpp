
#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <algorithm>

enum {
	UTF8ERR_INVALID=-1,
	UTF8ERR_NO_ROOM=-2
};

#ifndef UNICODE_BOM
#define UNICODE_BOM 0xFEFF
#endif

typedef char utf8_t;
typedef uint16_t utf16_t;

int utf8_encode(char **ptr,char *fence,uint32_t code) {
	int uchar_size=1;
	char *p = *ptr;

	if (!p) return UTF8ERR_NO_ROOM;
	if (code >= (uint32_t)0x80000000UL) return UTF8ERR_INVALID;
	if (p >= fence) return UTF8ERR_NO_ROOM;

	if (code >= 0x4000000) uchar_size = 6;
	else if (code >= 0x200000) uchar_size = 5;
	else if (code >= 0x10000) uchar_size = 4;
	else if (code >= 0x800) uchar_size = 3;
	else if (code >= 0x80) uchar_size = 2;

	if ((p+uchar_size) > fence) return UTF8ERR_NO_ROOM;

	switch (uchar_size) {
		case 1:	*p++ = (char)code;
			break;
		case 2:	*p++ = (char)(0xC0 | (code >> 6));
			*p++ = (char)(0x80 | (code & 0x3F));
			break;
		case 3:	*p++ = (char)(0xE0 | (code >> 12));
			*p++ = (char)(0x80 | ((code >> 6) & 0x3F));
			*p++ = (char)(0x80 | (code & 0x3F));
			break;
		case 4:	*p++ = (char)(0xF0 | (code >> 18));
			*p++ = (char)(0x80 | ((code >> 12) & 0x3F));
			*p++ = (char)(0x80 | ((code >> 6) & 0x3F));
			*p++ = (char)(0x80 | (code & 0x3F));
			break;
		case 5:	*p++ = (char)(0xF8 | (code >> 24));
			*p++ = (char)(0x80 | ((code >> 18) & 0x3F));
			*p++ = (char)(0x80 | ((code >> 12) & 0x3F));
			*p++ = (char)(0x80 | ((code >> 6) & 0x3F));
			*p++ = (char)(0x80 | (code & 0x3F));
			break;
		case 6:	*p++ = (char)(0xFC | (code >> 30));
			*p++ = (char)(0x80 | ((code >> 24) & 0x3F));
			*p++ = (char)(0x80 | ((code >> 18) & 0x3F));
			*p++ = (char)(0x80 | ((code >> 12) & 0x3F));
			*p++ = (char)(0x80 | ((code >> 6) & 0x3F));
			*p++ = (char)(0x80 | (code & 0x3F));
			break;
	};

	*ptr = p;
	return 0;
}

int utf8_decode(const char **ptr,const char *fence) {
	const char *p = *ptr;
	int uchar_size=1;
	int ret = 0,c;

	if (!p) return UTF8ERR_NO_ROOM;
	if (p >= fence) return UTF8ERR_NO_ROOM;

	ret = (unsigned char)(*p);
	if (ret >= 0xFE) { p++; return UTF8ERR_INVALID; }
	else if (ret >= 0xFC) uchar_size=6;
	else if (ret >= 0xF8) uchar_size=5;
	else if (ret >= 0xF0) uchar_size=4;
	else if (ret >= 0xE0) uchar_size=3;
	else if (ret >= 0xC0) uchar_size=2;
	else if (ret >= 0x80) { p++; return UTF8ERR_INVALID; }

	if ((p+uchar_size) > fence)
		return UTF8ERR_NO_ROOM;

	switch (uchar_size) {
		case 1:	p++;
			break;
		case 2:	ret = (ret&0x1F)<<6; p++;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= c&0x3F;
			break;
		case 3:	ret = (ret&0xF)<<12; p++;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<6;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= c&0x3F;
			break;
		case 4:	ret = (ret&0x7)<<18; p++;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<12;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<6;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= c&0x3F;
			break;
		case 5:	ret = (ret&0x3)<<24; p++;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<18;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<12;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<6;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= c&0x3F;
			break;
		case 6:	ret = (ret&0x1)<<30; p++;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<24;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<18;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<12;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= (c&0x3F)<<6;
			c = (unsigned char)(*p++); if ((c&0xC0) != 0x80) return UTF8ERR_INVALID;
			ret |= c&0x3F;
			break;
	};

	*ptr = p;
	return ret;
}

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

            if (fstatat(dirfd(dir), d->d_name, &st, AT_SYMLINK_NOFOLLOW)) continue;
            if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) continue;

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

    printf("%s\n",ent.first.c_str());
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
    int col1 = 10;
    const char *s;
    char tmp[64];
    std::string tmp2;

    {
        char c = 'F';

        if (S_ISLNK(ent.second.st_mode))
            c = 'L';
        else if (S_ISDIR(ent.second.st_mode))
            c = 'D';

        sprintf(tmp,"%c %llu",c,(unsigned long long)ent.second.st_size);
    }

    s = tmp;
    while (c < col1) {
        if (*s != 0)
            temp_render[c++] = *s++;
        else
            temp_render[c++] = ' ';
    }

    temp_render[c] = 0;
    tmp2 = temp_render;
    tmp2 += " ";
    tmp2 += ent.first;

    printf("%s\n",tmp2.c_str());
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

void find_file_dir(const std::string &name) {
    size_t i = 0;

    while (i < dirlist.size()) {
        dirlist_entry_t &ent = dirlist[i];

        if (name == ent.first) {
            dirlist_sel = i;
            dirlist_scroll = i;
            break;
        }

        i++;
    }
}

void accept_file(const std::string &name) {
    // TODO
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
        else if (in == "\x1B[1~") { /* home */
            size_t adj = screen_rows - TOPLIST_ROW;

            if (dirlist_sel > 0) {
		dirlist_sel = 0;
                dirlist_scroll = 0;
                redraw = 1;
            }
        }
        else if (in == "\x1B[5~") { /* page up */
            size_t adj = screen_rows - TOPLIST_ROW;

            if (dirlist_sel > 0) {
                if (dirlist_sel >= adj)
                    dirlist_sel -= adj;
                else
                    dirlist_sel = 0;

                dirlist_scroll = dirlist_sel;
                redraw = 1;
            }
        }
        else if (in == "A") {
            if (dirlist.size() != 0) {
                printf("\x1B[0m");
                printf("\x1B[2J");
                printf("\x1B[H");
                printf("Accept '%s'?",dirlist[dirlist_sel].first.c_str());
                fflush(stdout);

                in = read_in();
                if (in == "y" || in == "Y") {
                    accept_file(dirlist[dirlist_sel].first);
                    scan_dir();
                }

                redraw = 1;
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
        else if (in == "\x1B[6~") { /* page down */
            size_t adj = screen_rows - TOPLIST_ROW;

            if (dirlist.size() != 0) {
                if (dirlist_sel < (dirlist.size() - 1u)) {
                    dirlist_sel += adj;
                    if (dirlist_sel > (dirlist.size() - 1u))
                        dirlist_sel = (dirlist.size() - 1u);

                    dirlist_scroll = dirlist_sel - (screen_rows - TOPLIST_ROW);
                    redraw = 1;
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

