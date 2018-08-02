
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

int main() {
    return 0;
}

