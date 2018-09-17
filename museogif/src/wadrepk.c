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

static char tmp[4096];

wadinfo_t       header;
filelump_t*     lumps = NULL;

int main(int argc,char **argv) {
    int ofs;
    int ofd;
    int fd;
    int c;
    int i;

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

    lumps = malloc(sizeof(filelump_t) * header.numlumps);
    if (lumps == NULL)
        return 1;

    if (read(fd,lumps,sizeof(filelump_t) * header.numlumps) != (sizeof(filelump_t) * header.numlumps))
        return 1;

    ofd = open("wad.tmp",O_WRONLY|O_CREAT|O_TRUNC,0644);
    if (ofd < 0)
        return 1;

    ofs = sizeof(header);
    for (i=0;i < header.numlumps;i++) {
        if (lseek(fd,lumps[i].filepos,SEEK_SET) != lumps[i].filepos)
            return 1;
        if (lseek(ofd,ofs,SEEK_SET) != ofs)
            return 1;

        c = lumps[i].size;
        while (c >= sizeof(tmp)) {
            read(fd,tmp,sizeof(tmp));
            write(ofd,tmp,sizeof(tmp));
            c -= sizeof(tmp);
        }

        if (c > 0) {
            read(fd,tmp,c);
            write(ofd,tmp,c);
            c = 0;
        }

        lumps[i].filepos = ofs;
        ofs += lumps[i].size;
    }
    header.infotableofs = ofs;

    if (lseek(ofd,0,SEEK_SET) != 0)
        return 1;
    if (write(ofd,&header,sizeof(header)) != sizeof(header))
        return 1;

    if (lseek(ofd,header.infotableofs,SEEK_SET) != header.infotableofs)
        return 1;
    if (write(ofd,lumps,sizeof(filelump_t) * header.numlumps) != (sizeof(filelump_t) * header.numlumps))
        return 1;

    free(lumps);
    close(ofd);
    close(fd);

    /* WARNING: This only works in Linux */
    rename("wad.tmp",argv[1]);

    return 0;
}

