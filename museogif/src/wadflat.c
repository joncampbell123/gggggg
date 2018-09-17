#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
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

#define _Little_Endian_

/* [doc] windows_BITMAPFILEHEADER
 *
 * Packed portable representation of the Microsoft Windows BITMAPFILEHEADER
 * structure.
 */
typedef struct {						/* (sizeof) (offset hex) (offset dec) */
	uint16_t _Little_Endian_	bfType;			/* (2) +0x00 +0 */
	uint32_t _Little_Endian_	bfSize;			/* (4) +0x02 +2 */
	uint16_t _Little_Endian_	bfReserved1;		/* (2) +0x06 +6 */
	uint16_t _Little_Endian_	bfReserved2;		/* (2) +0x08 +8 */
	uint32_t _Little_Endian_	bfOffBits;		/* (4) +0x0A +10 */
} __attribute__((packed)) windows_BITMAPFILEHEADER;		/* (14) =0x0E =14 */

typedef struct {						/* (sizeof) (offset hex) (offset dec) */
	uint32_t _Little_Endian_	biSize;			/* (4) +0x00 +0 */
	int32_t _Little_Endian_		biWidth;		/* (4) +0x04 +4 */
	int32_t _Little_Endian_		biHeight;		/* (4) +0x08 +8 */
	uint16_t _Little_Endian_	biPlanes;		/* (2) +0x0C +12 */
	uint16_t _Little_Endian_	biBitCount;		/* (2) +0x0E +14 */
	uint32_t _Little_Endian_	biCompression;		/* (4) +0x10 +16 */
	uint32_t _Little_Endian_	biSizeImage;		/* (4) +0x14 +20 */
	int32_t _Little_Endian_		biXPelsPerMeter;	/* (4) +0x18 +24 */
	int32_t _Little_Endian_		biYPelsPerMeter;	/* (4) +0x1C +28 */
	uint32_t _Little_Endian_	biClrUsed;		/* (4) +0x20 +32 */
	uint32_t _Little_Endian_	biClrImportant;		/* (4) +0x24 +36 */
} __attribute__((packed)) windows_BITMAPINFOHEADER;		/* (40) =0x28 =40 */

int main(int argc,char **argv) {
    int ofs;
    int fd;
    int c;
    int i;
    int nent;
    int need_fend=1;
    int need_fstart=1;

    if (argc < 4) {
        fprintf(stderr,"wadflat <WAD> <NAME> <64x64 BMP FILE>\n");
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

    lumps = malloc(sizeof(filelump_t) * (header.numlumps + 3)); /* add up to 3: the flat, and possible F_START/F_END */
    if (lumps == NULL)
        return 1;

    if (read(fd,lumps,sizeof(filelump_t) * header.numlumps) != (sizeof(filelump_t) * header.numlumps))
        return 1;

    nent = header.numlumps;
    for (i=0;i < header.numlumps;i++) {
        memcpy(tmp,lumps[i].name,8); tmp[8] = 0;
        if (!strcmp(tmp,"F_START")) need_fstart = 0;
        if (!need_fstart && !strcmp(tmp,"F_END")) {
            need_fend = 0;
            nent = i; /* insert here */
            break;
        }
    }

    ofs = lseek(fd,0,SEEK_END);

    if (need_fstart) {
        assert(nent == header.numlumps);
        memset(&lumps[header.numlumps],0,sizeof(filelump_t));
        strcpy(lumps[header.numlumps].name,"F_START");
        lumps[header.numlumps].filepos = ofs;
        lumps[header.numlumps].size = 0;
        header.numlumps++;
        nent = header.numlumps;
    }

    lumps[nent].filepos = ofs;
    lumps[nent].size = 64*64;
    ofs += 64*64;
    if (nent == header.numlumps) header.numlumps++;

    if (need_fend) {
        assert((nent+1) == header.numlumps);
        memset(&lumps[header.numlumps],0,sizeof(filelump_t));
        strcpy(lumps[header.numlumps].name,"F_END");
        lumps[header.numlumps].filepos = ofs;
        lumps[header.numlumps].size = 0;
        header.numlumps++;
    }

    {
        char *d = lumps[nent].name;
        char *s = argv[2];
        int x = 0;

        while (x < 8 && *s != 0) {
            *d++ = *s++;
            x++;
        }
        while (x < 8) {
            *d++ = 0;
            x++;
        }
    }

    {
        int by,bx;
        char *flat = malloc(64*64);
        int bmp_fd = open(argv[3],O_RDONLY | O_BINARY);
        if (bmp_fd < 0) {
            fprintf(stderr,"Failed to open %s\n",argv[3]);
            return 1;
        }

        windows_BITMAPFILEHEADER bhdr;
        windows_BITMAPINFOHEADER ihdr;

        if (lseek(fd,lumps[nent].filepos,SEEK_SET) != lumps[nent].filepos)
            return 1;

        if (read(bmp_fd,&bhdr,sizeof(bhdr)) != sizeof(bhdr))
            return 1;
        if (bhdr.bfType != 0x4D42) {
            fprintf(stderr,"bfType wrong\n");
            return 1;
        }
        if (read(bmp_fd,&ihdr,sizeof(ihdr)) != sizeof(ihdr))
            return 1;
        if (ihdr.biSize < sizeof(windows_BITMAPINFOHEADER)) {
            fprintf(stderr,"wrong biSize\n");
            return 1;
        }
        if (ihdr.biWidth != 64 || ihdr.biHeight != 64) {
            fprintf(stderr,"wrong biWidth/biHeight\n");
            return 1;
        }
        if (ihdr.biPlanes != 1 || ihdr.biBitCount != 8 || ihdr.biCompression != 0) {
            fprintf(stderr,"wrong biPlanes, biBitCount, biCompression\n");
            return 1;
        }
        if (lseek(bmp_fd,bhdr.bfOffBits,SEEK_SET) != bhdr.bfOffBits)
            return 1;
        if (read(bmp_fd,flat,64*64) != (64*64))
            return 1;

        /* flip the image vertically before accepting.
         * BMPs are upside down */
        for (by=0;by < (64/2);by++) {
            char *r1 = flat + (by * 64);
            char *r2 = flat + ((63 - by) * 64);
            for (bx=0;bx < 64;bx++) {
                char tmp = *r1;
                *r1 = *r2;
                *r2 = tmp;
            }
        }

        if (write(fd,flat,64*64) != (64*64))
            return 1;

        close(bmp_fd);
        free(flat);
    }

    header.infotableofs = ofs;

    if (lseek(fd,0,SEEK_SET) != 0)
        return 1;
    if (write(fd,&header,sizeof(header)) != sizeof(header))
        return 1;

    if (lseek(fd,header.infotableofs,SEEK_SET) != header.infotableofs)
        return 1;
    if (write(fd,lumps,sizeof(filelump_t) * header.numlumps) != (sizeof(filelump_t) * header.numlumps))
        return 1;

    free(lumps);
    close(fd);
    return 0;
}

