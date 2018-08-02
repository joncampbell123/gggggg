
#include <sys/types.h>
#include <sys/stat.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <string>
#include <vector>

std::vector< std::pair<std::string,struct stat> >       dirlist;
int                                                     dirlist_sel = -1;

struct termios                                          oterm,nterm;

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
    tcgetattr(0/*STDIN*/,&oterm);
    nterm = oterm;
    nterm.c_lflag &= ~ICANON;
    tcsetattr(0/*STDIN*/,TCSANOW,&nterm);



    tcsetattr(0/*STDIN*/,TCSANOW,&oterm);
    return 0;
}

