#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "doomtype.h"
#include "w_wad.h"

#ifndef O_BINARY
#define O_BINARY (0)
#endif

static char tmp[256];

wadinfo_t       header;
filelump_t*     lumps = NULL;

int main(int argc,char **argv) {
    int fd;
    int c;
    int i;

    if (argc < 3) {
        fprintf(stderr,"Must specify WAD file and what to delete\n");
        return 1;
    }

    fd = open(argv[1],O_RDWR | O_BINARY);
    if (fd < 0) 
        return 1;
    if (read(fd,&header,sizeof(header)) != sizeof(header))
        return 1;

    if (memcmp(header.identification,"PWAD",4) && memcmp(header.identification,"IWAD",4)) {
        fprintf(stderr,"Invalid WAD\n");
        return 1;
    }

    memcpy(tmp,header.identification,4); tmp[4]=0;
    printf("WAD header: '%s' lumps=%u offset=%u\n",tmp,header.numlumps,header.infotableofs);

    if (lseek(fd,header.infotableofs,SEEK_SET) != header.infotableofs)
        return 1;

    lumps = malloc(sizeof(filelump_t) * header.numlumps);
    if (lumps == NULL)
        return 1;

    if (read(fd,lumps,sizeof(filelump_t) * header.numlumps) != (sizeof(filelump_t) * header.numlumps))
        return 1;

    for (c=2;c < argc;c++) {
        char *s = argv[c];
        int i = strtol(s,&s,10);
        int j = i;

        if (*s == '-') {
            s++;
            j = strtol(s,&s,10);
            if (j < i) j = i;
        }

        if (j >= header.numlumps)
            j = header.numlumps - 1;

        if (i >= 0 && i < header.numlumps) {
            while (i <= j) {
                memcpy(tmp,lumps[i].name,8); tmp[8] = 0;
                memset(&lumps[i],0,sizeof(filelump_t));
                printf("Removing entry %d, '%s'\n",i,tmp);
                i++;
            }
        }
    }

    if (lseek(fd,header.infotableofs,SEEK_SET) != header.infotableofs)
        return 1;

    if (write(fd,lumps,sizeof(filelump_t) * header.numlumps) != (sizeof(filelump_t) * header.numlumps))
        return 1;

    free(lumps);
    close(fd);
    return 0;
}

