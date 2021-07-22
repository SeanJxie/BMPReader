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

BOOL read_pixel_data(struct BMPHeaders* pBmpHeaders, BYTE* pixelDataBuffer, UINT* colMapBuffer, FILE* pFileStream) {
    /* TODO Color table handling */
    if (pBmpHeaders->imBitDepth != (USHORT)24) {
        printf("%d-bit BMPs are not yet supported.\n", pBmpHeaders->imBitDepth);
        return FALSE;
    } 

    /* Read the pixel data to the buffer. */
    if (fseek(pFileStream, pBmpHeaders->pixelDataOffset, SEEK_SET) == 0) {
        /* The number of bytes read should be the size of the image. */
        size_t bytesRead = fread(pixelDataBuffer, sizeof(BYTE), pBmpHeaders->fileByteSize - pBmpHeaders->pixelDataOffset, pFileStream);
        if ((pBmpHeaders->fileByteSize - pBmpHeaders->pixelDataOffset) - bytesRead > 2) {
            return FALSE;
        }
    } else {
        return FALSE;
    }
    return TRUE;
}

BOOL write_bmp_to_screen(struct BMPHeaders* pBmpHeaders, BYTE* pixelData, UINT* colMap) {
    HDC hMon = GetDC(NULL);

    /* 24-bit pixel data. */
    if (pBmpHeaders->imBitDepth == (USHORT)24) {
        UINT x = 0, y = pBmpHeaders->imHt;
        for (UINT i = 0; i < pBmpHeaders->fileByteSize; i += 3) {
            if (SetPixel(hMon, x, y, RGB(pixelData[i + 2], pixelData[i + 1], pixelData[i])) == -1) {
                return FALSE;
            } else {
                x++;
                if (x == pBmpHeaders->imWt) { x = 0; y--; }
            }
        }
    }

    /* 8-bit pixel data. */
    else if (pBmpHeaders->imBitDepth <= (USHORT)8) {
        UINT x = 0, y = pBmpHeaders->imHt;
        for (UINT i = 0; i < pBmpHeaders->fileByteSize; i++) {
            if (SetPixel(hMon, x, y, (COLORREF)colMap[(UINT)pixelData[i]]) == -1) {
                return FALSE;
            } else {
                x++;
                if (x == pBmpHeaders->imWt) { x = 0; y--; }
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
    printf("Bit depth         : %d \n"     , pBmp->imBitDepth);
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
    
    struct BMPHeaders bmp;
    if (read_bmp_header(&bmp, pBmpFile)) {
        show_bmp_info(&bmp);  
    } 
    else {
        printf("Cannot read file.\n");
        return EXIT_FAILURE;
    }

    BYTE* pixelData = malloc(bmp.fileByteSize);
    UINT* colMap    = malloc(bmp.imColorMapEntriesUsed * sizeof(UINT)); /* Each col map entry is 4 bytes. */

    if (read_pixel_data(&bmp, pixelData, colMap, pBmpFile)) {
        write_bmp_to_screen(&bmp, pixelData, colMap);
    } 
    else {
        printf("Cannot read pixel data.\n");
        return EXIT_FAILURE;
    }

    free(pixelData);
    free(colMap);
    fclose(pBmpFile);

    return EXIT_SUCCESS;
}