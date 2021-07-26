#include <stdio.h>
#include <Windows.h>

/*
A program to read and dump BMP data onto the screen.
- Sean Xie
*/

/* 
#pragma directives to disable struct padding. 
https://en.wikipedia.org/wiki/Data_structure_alignment
*/

#pragma pack(push, 1)
struct BMPHeaders {
    /* Handy reference: https://cdn.hackaday.io/files/274271173436768/Simplified%20Windows%20BMP%20Bitmap%20File%20Format%20Specification.htm */

    /* File header */
    USHORT fileSignature;
    UINT   fileByteSize;
    USHORT res1;
    USHORT res2;
    UINT   pixelDataOffset;

    /* Image header */
    UINT   imHeaderSize;
    UINT   imWt;
    UINT   imHt;
    USHORT imPlanes;
    USHORT imBitDepth;
    UINT   imCompression;
    UINT   imSize;
    UINT   imXPixelsPerMeter;
    UINT   imYPixelsPerMeter;
    UINT   imColorMapEntriesUsed;
    UINT   imImportantCols;
};
#pragma pack(pop)

BOOL read_bmp_header(struct BMPHeaders* pBmpBuffer, FILE* pFileStream) {
    /* Read BMP header to buffer. */
    if (fread(pBmpBuffer, sizeof(BYTE), sizeof(*pBmpBuffer), pFileStream) != sizeof(*pBmpBuffer)) {
        return FALSE;
    }
    return TRUE;
}

BOOL read_pixel_data(struct BMPHeaders* pBmpHeaders, BYTE* pixelDataBuffer, BYTE* colMapBuffer, FILE* pFileStream) {
    /* Color table handling */
    if (pBmpHeaders->imBitDepth == (USHORT)0x8) {
        if (fread(colMapBuffer, sizeof(BYTE), (size_t)(0x1 << pBmpHeaders->imBitDepth) * 0x4, pFileStream) != (size_t)(0x1 << pBmpHeaders->imBitDepth) * 0x4) {
            return FALSE;
        }
    } else if (pBmpHeaders->imBitDepth == (USHORT)0x1) {
        if (fread(colMapBuffer, sizeof(BYTE), (size_t)0x8, pFileStream) != (size_t)0x8) {
            return FALSE;
        }
    } else if (pBmpHeaders->imBitDepth != (USHORT)0x18) {
        printf("%d-bit BMPs are not yet supported.\n", pBmpHeaders->imBitDepth);
    }

    /* Read the pixel data to the buffer. */
    if (fseek(pFileStream, pBmpHeaders->pixelDataOffset, SEEK_SET) == 0x0) {
        /* The number of bytes read should be the size of the image. */
        size_t bytesRead = fread(pixelDataBuffer, sizeof(BYTE), pBmpHeaders->fileByteSize - pBmpHeaders->pixelDataOffset, pFileStream);
        if ((pBmpHeaders->fileByteSize - pBmpHeaders->pixelDataOffset) - bytesRead > 0x2) {
            return FALSE;
        }
    } else {
        return FALSE;
    }
    return TRUE;
}

BOOL write_bmp_to_screen(struct BMPHeaders* pBmpHeaders, BYTE* pixelData, BYTE* colMap) {
    HDC hMon = GetDC(NULL);

    /* 24-bit pixel data. */
    if (pBmpHeaders->imBitDepth == (USHORT)0x18) {
        UINT x = 0, y = pBmpHeaders->imHt;
        for (UINT i = 0x0; i < pBmpHeaders->imSize; i += 0x3) {
            if (SetPixel(hMon, x, y, RGB(pixelData[i + 0x2], pixelData[i + 0x1], pixelData[i])) == -1) {
                return FALSE;
            } else {
                x++;
                if (x >= pBmpHeaders->imWt) { x = 0; y--; }
            }
        }
    }
    
    /* 8-bit pixel data. */
    else if (pBmpHeaders->imBitDepth == (USHORT)0x8) {
        UINT x = 0x0, y = pBmpHeaders->imHt, _temp;
        for (UINT i = 0x0; i < pBmpHeaders->imSize; i++) {
            _temp = pixelData[i] * 0x4;
            if (SetPixel(hMon, x, y, RGB(colMap[_temp + 0x2], colMap[_temp + 0x1], colMap[_temp])) == -1) {
                return FALSE;
            } else {
                x++;
                if (x >= pBmpHeaders->imWt) { x = 0x0; y--; }
            }
        }
    }

    /* 1-bit pixel data. */
    else if (pBmpHeaders->imBitDepth == (USHORT)0x1) {
        UINT x = 0x0, y = pBmpHeaders->imHt, _temp, _bitIsSetIdx;
        for (UINT i = 0x0; i < pBmpHeaders->imSize; i++) {
            _temp = pixelData[i];
            for (UINT j = 0x0; j < 0x8; j++) {
                _bitIsSetIdx = ((_temp & (1 << j)) != 0) * 0x4;
                if (SetPixel(hMon, x, y, RGB(colMap[_bitIsSetIdx + 0x2], colMap[_bitIsSetIdx + 0x1], colMap[_bitIsSetIdx])) == -1) {
                    return FALSE;
                } else {
                    x++;
                    if (x >= pBmpHeaders->imWt) { x = 0x0; y--; }
                }
            }
        }
    }

    return TRUE;
}

void show_bmp_info(struct BMPHeaders* pBmp) {
    printf("Header struct size: %d bytes\n\n", (int)sizeof(*pBmp));

    printf("File signature    : %x \n"       , pBmp->fileSignature);
    printf("Byte size         : %d bytes\n"  , pBmp->fileByteSize);
    printf("Pixel data offset : %d bytes\n\n", pBmp->pixelDataOffset);

    printf("Image header size : %d bytes\n", pBmp->imHeaderSize);
    printf("Image width       : %d px\n"   , pBmp->imHt);
    printf("Image height      : %d px\n"   , pBmp->imWt);
    printf("Color planes      : %d \n"     , pBmp->imPlanes);
    printf("Bit depth         : %d bit\n"  , pBmp->imBitDepth);
    printf("Compression type  : %d \n"     , pBmp->imCompression);
    printf("Image size        : %d bytes\n", pBmp->imSize);
    printf("Image ppm (x)     : %d px\n"   , pBmp->imXPixelsPerMeter);
    printf("Image ppm (y)     : %d px\n"   , pBmp->imYPixelsPerMeter);
    printf("Cols used         : %d \n"     , pBmp->imColorMapEntriesUsed);
    printf("Important cols    : %d \n\n"   , pBmp->imImportantCols);
}

int main(int argc, char* argv[]) {
    if (argc == 1) { 
        printf("Missing command line argument (BMP file name).\n");
        return EXIT_FAILURE; 
    }
    FILE* pBmpFile = fopen(argv[1], "rb");
    if (pBmpFile == NULL) {
        printf("File %s does not exist.\n", argv[1]);
    }
    
    struct BMPHeaders bmph;
    if (read_bmp_header(&bmph, pBmpFile)) {
        show_bmp_info(&bmph);  
    } 
    else {
        printf("Cannot read file.\n");
        return EXIT_FAILURE;
    }

    if (bmph.imCompression == 0) {
        BYTE* pixelData = malloc(bmph.fileByteSize);
        BYTE* colMap    = malloc((0x1 << bmph.imBitDepth) * 0x4); /* Each col map entry is 4 bytes. */

        if (read_pixel_data(&bmph, pixelData, colMap, pBmpFile)) {
            if (!write_bmp_to_screen(&bmph, pixelData, colMap)) {
                printf("Cannot write to screen.\n");
            }
        } 
        else {
            printf("Cannot read pixel data.\n");
            return EXIT_FAILURE;
        }

        free(pixelData);
        free(colMap);

    } else {
        printf("Cannot read compressed BMP.\n");
    }
    
    fclose(pBmpFile);
    return EXIT_SUCCESS;
}