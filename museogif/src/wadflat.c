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

    lumps = malloc(sizeof(filelump_t) * (header.numlumps + 1));
    if (lumps == NULL)
        return 1;

    if (read(fd,lumps,sizeof(filelump_t) * header.numlumps) != (sizeof(filelump_t) * header.numlumps))
        return 1;

    ofs = lseek(fd,0,SEEK_END);
    lumps[header.numlumps].filepos = ofs;
    lumps[header.numlumps].size = 64*64;
    {
        char *d = lumps[header.numlumps].name;
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

    ofs += lumps[header.numlumps].size;
    header.numlumps++;

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

