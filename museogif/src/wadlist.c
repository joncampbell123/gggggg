#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "doomtype.h"
#include "w_wad.h"

#ifndef O_BINARY
#define O_BINARY (0)
#endif

static char tmp[256];

int main(int argc,char **argv) {
    wadinfo_t header;
    filelump_t lump;
    int fd;
    int c;

    if (argc < 2) {
        fprintf(stderr,"Must specify WAD file\n");
        return 1;
    }

    fd = open(argv[1],O_RDONLY | O_BINARY);
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

    for (c=0;c < header.numlumps;c++) {
        if (read(fd,&lump,sizeof(lump)) != sizeof(lump))
            break;

        memcpy(tmp,lump.name,8); tmp[8] = 0;
        printf(" Entry %d: filepos=%u size=%u name='%s'\n",c,lump.filepos,lump.size,tmp);
    }

    close(fd);
    return 0;
}

