#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>
#include "cuefileParser.h"

// Platform-independent helper macros
#define MAX_PATH 4096
#define UNREFERENCED_PARAMETER(P) (P)

// Utility functions for path handling on macOS
void splitPath(const char* path, char* dir, char* fname, char* ext) {
    char* dname = dirname(strdup(path));
    char* bname = basename(strdup(path));
    
    // Get directory
    strcpy(dir, dname);
    strcat(dir, "/");
    
    // Get extension
    char* dot = strrchr(bname, '.');
    if (dot != NULL) {
        strcpy(ext, dot);
        *dot = '\0';  // Truncate fname at dot
        strcpy(fname, bname);
    } else {
        ext[0] = '\0';
        strcpy(fname, bname);
    }
    
    free(dname);
    free(bname);
}

void makePath(char* path, const char* dir, const char* fname, const char* ext) {
    strcpy(path, dir);
    strcat(path, fname);
    if (ext && ext[0]) {
        strcat(path, ext);
    }
}

// Core utility functions
int addh() {
    FILE* fin = fopen("input-raw-right2.raw", "rb");
    if (!fin) {
        printf("ERROR: Cannot open input file!\n");
        exit(2);
    }
    
    FILE* fout = fopen("input-right2.wav", "wb");
    if (!fout) {
        fclose(fin);
        printf("ERROR: Cannot create output file!\n");
        exit(2);
    }

    // Get input file size
    fseek(fin, 0, SEEK_END);
    long finSize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    // WAV header constants
    const char riff[] = "RIFF";
    const char wave[] = "WAVE";
    const char fmt[] = "fmt ";
    const char data[] = "data";
    
    // WAV format parameters
    const long fmtSize = 16;
    const int audioFmt = 1;  // PCM
    const int numChannels = 1;
    const long sampleRate = 44100;
    const long byteRate = 44100;
    const int blockAlign = 1;
    const int bitsPerSample = 8;
    
    // Calculate sizes
    long dataSize = finSize;
    long fileSize = dataSize + 36;  // Total file size minus 8 bytes for RIFF header
    
    // Write WAV header
    fwrite(riff, 1, 4, fout);
    fwrite(&fileSize, 4, 1, fout);
    fwrite(wave, 1, 4, fout);
    fwrite(fmt, 1, 4, fout);
    fwrite(&fmtSize, 4, 1, fout);
    fwrite(&audioFmt, 2, 1, fout);
    fwrite(&numChannels, 2, 1, fout);
    fwrite(&sampleRate, 4, 1, fout);
    fwrite(&byteRate, 4, 1, fout);
    fwrite(&blockAlign, 2, 1, fout);
    fwrite(&bitsPerSample, 2, 1, fout);
    fwrite(data, 1, 4, fout);
    fwrite(&dataSize, 4, 1, fout);
    
    // Copy audio data
    char* buffer = (char*)malloc((size_t)finSize);
    if (buffer) {
        fread(buffer, 1, (size_t)finSize, fin);
        fwrite(buffer, 1, (size_t)finSize, fout);
        free(buffer);
    }
    
    fclose(fin);
    fclose(fout);
    unlink("input-raw-right2.raw");  // Delete temporary file
    return 0;
}

// Header detection functions
int isHeader(unsigned char buf) {
    return (buf == 0xe1 || buf == 0xc3 || buf == 0xa5) ? 1 : 0;
}

int isFooter(unsigned char buf) {
    return (buf == 0xd2 || buf == 0xb4 || buf == 0x96) ? 1 : 0;
}

int isColor(unsigned char* buf) {
    return (buf[0] == 0x81 && buf[1] == 0xe3 && buf[2] == 0xe3 &&
            buf[3] == 0xc7 && buf[4] == 0xc7 && buf[5] == 0x81 &&
            buf[6] == 0x81 && buf[7] == 0xe3 && buf[8] == 0xc7) ? 1 : 0;
}

//Continue with the video processing functions.

// Frame extraction helpers
size_t fseekToHeader(FILE* fp) {
    size_t size = 0;
    unsigned char buf = 0;
    
    while (!feof(fp) && !ferror(fp)) {
        size += fread(&buf, 1, 1, fp);
        if (isHeader(buf)) {
            for (;;) {
                size += fread(&buf, 1, 1, fp);
                if (!isHeader(buf)) {
                    fseek(fp, -1, SEEK_CUR);
                    size--;
                    break;
                }
            }
            if (0 < size && size < 1340) {
                long size2 = (long)(1340 - size - 1);
                fseek(fp, size2, SEEK_CUR);
                fread(&buf, 1, 1, fp);

                if (!isHeader(buf)) {
                    fseek(fp, -size2 - 1, SEEK_CUR);
                }
                else {
                    size += size2;
                }
            }
            if (isFooter(buf)) {
                fseek(fp, 1339, SEEK_CUR);
                size += 1339;
                break;
            }
            break;
        }
    }
    return size;
}

int xtract(long* ts) {
    FILE* fin = fopen("input-raw-left.raw", "rb");
    if (!fin) {
        return 1;
    }

    // Get file size
    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    rewind(fin);

    // Skip pregap and first header
    size_t seeksize = fseekToHeader(fin);
    printf("Seek size: %zd\n", seeksize);
    size = (long)(size - seeksize - 1340);

    // Process frames
    for (long tt = size / 5880; tt > 0; tt--) {
        *ts += 1;
        printf("\rProcessing frame number %li", *ts);
        
        char fname[32];
        snprintf(fname, sizeof(fname), "%li.pgm", *ts);
        FILE* fout = fopen(fname, "wb");
        if (!fout) continue;

        fputs("P5\n40 80\n255\n", fout);
        
        for (int times = 3200; times != 0; times--) {
            unsigned char buffer;
            fread(&buffer, 1, 1, fin);
            
            if (buffer == 0xe1) {
                unsigned char nextBuf = 0;
                fread(&nextBuf, 1, 1, fin);
                if (nextBuf == 0xe1) {
                    fputc(0, fout);
                    printf("\n%li.pgm %d: padded by 0", *ts, 3200 - times);
                    fseek(fin, -2, SEEK_CUR);
                }
                else {
                    fwrite(&buffer, 1, 1, fout);
                    fseek(fin, -1, SEEK_CUR);
                }
            }
            else {
                fwrite(&buffer, 1, 1, fout);
            }
        }
        fclose(fout);

        // Skip footer
        unsigned char buffer;
        do {
            fread(&buffer, 1, 1, fin);
        } while (isFooter(buffer));
        fseek(fin, -1, SEEK_CUR);

        // Check next header
        fread(&buffer, 1, 1, fin);
        if (isHeader(buffer)) {
            fseek(fin, 1339, SEEK_CUR);
        }
    }
    printf("\n");
    fclose(fin);
    unlink("input-raw-left.raw");
    return 0;
}

int addhColorXp(long frmSize, long audioSizePerFrm, size_t seekPos)
{
    UNREFERENCED_PARAMETER(seekPos);
    FILE* fin;
    FILE* fout;
    long finSize;
    char buffer;
    fin = fopen("input-raw-right2.raw", "rb");
    if (!fin) {
        printf("ERROR!");
        exit(2);
    }
    char riff[] = "RIFF";
    char wave[] = "WAVE";
    char fmt[] = "fmt ";
    long sc1size = 16;
    int audiofmt = 1;
    int numchanels = 2;
    long smprate = 17640;
    long byterate = 35280;
    int blkalign = 1;
    int bits = 8;
    char dataa[] = "data";

    fout = fopen("input-right2.wav", "wb");
    fwrite(riff, 4, 1, fout);
    fseek(fin, 0, 2);
    finSize = frmSize * audioSizePerFrm;
    fseek(fin, 0, 0);

    long finSizea;
    finSizea = finSize + 40;
    fwrite(&finSizea, 4, 1, fout);
    fwrite(wave, 4, 1, fout);
    fwrite(fmt, 4, 1, fout);
    fwrite(&sc1size, 4, 1, fout);
    fwrite(&audiofmt, 2, 1, fout);
    fwrite(&numchanels, 2, 1, fout);
    fwrite(&smprate, 4, 1, fout);
    fwrite(&byterate, 4, 1, fout);
    fwrite(&blkalign, 2, 1, fout);
    fwrite(&bits, 2, 1, fout);
    fwrite(dataa, 4, 1, fout);
    fwrite(&finSize, 4, 1, fout);

    long tt = 0;
    for (tt = frmSize; tt > 0; tt--) {
        for (int i = 0; i < audioSizePerFrm; i++) {
            fread(&buffer, 1, 1, fin);
            fwrite(&buffer, 1, 1, fout);
        }
    }
    fclose(fin);
    fclose(fout);
    remove("input-raw-right2.raw");
    return 0;
}

int outputPpm(unsigned char* video, int* idx, long ts) {
    char fname[32];
    FILE* fout;
    char writeBuf[12] = {};
    int nextline = 108;

    printf("\rProcessing frame number %li", ts);
    snprintf(fname, sizeof(fname), "%li.ppm", ts);
    fout = fopen(fname, "wb");
    if (!fout) return 1;

    fputs("P6\n144 80\n255\n", fout);

    int nRoop = 0;
    for (int i = 0; i < 17280 / 6; i++) {
        writeBuf[0] = video[*idx] & 0x0f | video[*idx] << 4 & 0xf0;
        writeBuf[1] = video[*idx + nextline] & 0x0f | video[*idx + nextline] << 4 & 0xf0;
        writeBuf[2] = video[*idx + nextline] & 0xf0 | video[*idx + nextline] >> 4 & 0x0f;

        writeBuf[3] = video[*idx + nextline + 1] & 0x0f | video[*idx + nextline + 1] << 4 & 0xf0;
        writeBuf[4] = video[*idx] & 0xf0 | video[*idx] >> 4 & 0x0f;
        writeBuf[5] = video[*idx + 1] & 0x0f | video[*idx + 1] << 4 & 0xf0;

        writeBuf[6] = video[*idx + 1] & 0xf0 | video[*idx] >> 4 & 0x0f;
        writeBuf[7] = video[*idx + nextline + 1] & 0xf0 | video[*idx + nextline + 1] >> 4 & 0x0f;
        writeBuf[8] = video[*idx + nextline + 2] & 0x0f | video[*idx + nextline + 2] << 4 & 0xf0;

        writeBuf[9] = video[*idx + nextline + 2] & 0xf0 | video[*idx + nextline + 2] >> 4 & 0x0f;
        writeBuf[10] = video[*idx + 2] & 0x0f | video[*idx + 2] << 4 & 0xf0;
        writeBuf[11] = video[*idx + 2] & 0xf0 | video[*idx + 2] >> 4 & 0x0f;

        fwrite(writeBuf, sizeof(writeBuf), 1, fout);

        *idx += 3;
        if (++nRoop == 36) {
            *idx += nextline;
            nRoop = 0;
        }
    }
    fclose(fout);
    return 0;
}

size_t fseekToColor(FILE* fp) {
    size_t size = 0;
    unsigned char buf[9] = {};
    
    while (!feof(fp) && !ferror(fp)) {
        size += fread(buf, 1, 9, fp);
        if (isColor(buf)) {
            fseek(fp, 351, SEEK_CUR);
            size += 351;
            break;
        }
        fseek(fp, -8, SEEK_CUR);
        size -= 8;
    }
    return size;
}

int xtractColor(long* frmSize, size_t* seekPos, long* ts) {
    struct stat stbuf;
    int fd = open("input-raw-left2.raw", O_RDONLY);
    if (fd == -1) return 1;
    
    if (fstat(fd, &stbuf) == -1) {
        close(fd);
        return 1;
    }

    FILE* fin = fdopen(fd, "rb");
    if (!fin) {
        close(fd);
        return 1;
    }

    unsigned char* buf = (unsigned char*)malloc((size_t)stbuf.st_size);
    if (!buf) {
        fclose(fin);
        return 1;
    }

    fread(buf, 1, (size_t)stbuf.st_size, fin);
    rewind(fin);

    *seekPos = fseekToColor(fin) - 360;
    printf("Seek size: %zd\n", *seekPos);
    long size = (long)(stbuf.st_size - *seekPos);

    *frmSize = (long)(size / 17640);
    unsigned char* video = &buf[*seekPos];
    int idx = 0;
    
    for (long tt = *frmSize; tt > 0; tt--) {
        idx += 360;
        outputPpm(video, &idx, ++(*ts));
    }
    
    printf("\n");
    free(buf);
    fclose(fin);
    unlink("input-raw-left2.raw");
    return 0;
}


size_t fseekToXp(FILE* fp)
{
    size_t size = 0;
    unsigned char buf[9] = {};
    while (!feof(fp) && !ferror(fp)) {
        size += fread(buf, 1, 9, fp);
        if (isColor(buf)) {
            fseek(fp, 495, SEEK_CUR);
            size += 495;
            break;
        }
        fseek(fp, -8, SEEK_CUR);
        size -= 8;
    }
    return size;
}

int xtractXp(long* frmSize, size_t* seekPos, long* ts)
{
    FILE * fin;
    long size;
    long tt;
    int fd;
    struct stat stbuf;

    fd = open("input-raw-left2.raw", O_RDONLY);
    if (!fd) {
        exit(1);
    }
    fin = fdopen(fd, "rb");
    if (!fin) {
        exit(1);
    }
    lseek(fd, stbuf.st_size, SEEK_SET); // Move to a specific position based on st_size


    unsigned char* buf = (unsigned char*)malloc((size_t)stbuf.st_size);
    if (!buf) {
        exit(1);
    }
    fread(buf, 1, (size_t)stbuf.st_size, fin);
    rewind(fin);

    *seekPos = fseekToXp(fin) - 504;
    printf("Seek size: %zd\n", *seekPos);
    size = (long)(stbuf.st_size - *seekPos);

    *frmSize = size / 17784;
    unsigned char* video = &buf[*seekPos];
    int idx = 0;
    for (tt = *frmSize; tt > 0; tt--) {
        idx += 504;
        outputPpm(video, &idx, ++(*ts));
    }
    printf("\n");
    fclose(fin);
    remove("input-raw-left2.raw");
    return 0;
}

int isColorOrXp(FILE* fp, long long* offset)
{
    unsigned char buf[9] = {};
    int nCnt = 0;
    while (!feof(fp) && !ferror(fp)) {
        *offset += fread(buf, 1, 9, fp);
        if (isColor(buf)) {
            nCnt++;
            for (int i = 0; i < 23; i++) {
                fread(buf, 1, 1, fp); // skip audio byte
                fread(buf, 1, 9, fp);
                if (isColor(buf)) {
                    nCnt++;
                }
            }
            *offset -= 9;
            fseeko(fp, (long)*offset, SEEK_SET);
            break;
        }
        fseek(fp, -8, SEEK_CUR);
        *offset -= 8;
    }
    return nCnt;
}

// Color and XP format handling
int handleColorOrXp(char* arg, FILE* fpImg, long fsize, int lba[MAX_TRACK][MAX_INDEX]) {
    FILE* foutr = fopen("input-raw-right2.raw", "wb");
    if (!foutr) return 1;

    unsigned char l1[9] = {};
    unsigned char r1;
    int type = 0;
    long frmSize = 0;
    int allFrmSize = 0;
    long ppmNum = 0;
    size_t seekPos = 0;

    for (int i = 1; lba[i][1] != -1; i++) {
        FILE* foutl = fopen("input-raw-left2.raw", "wb");
        if (!foutl) {
            fclose(foutr);
            return 1;
        }

        printf("Track[%02d] LBA: %6d, ", i, lba[i - 1][1]);
        off_t offset = (off_t)lba[i - 1][1] * 2352;
        fseeko(fpImg, offset, SEEK_SET);

        type = isColorOrXp(fpImg, &offset);
        frmSize = 0;
        if (type == 24) {
            printf("color ");
            frmSize = 19600;
        }
        else if (type == 12) {
            printf("   xp ");
            frmSize = 19760;
        }
        else {
            printf("Frames: %5d\n", 0);
            fclose(foutl);
            break;
        }

        int sectorNum = lba[i][1] - lba[i - 1][1];
        if (sectorNum == 0) {
            sectorNum = (int)(fsize / 2352 - lba[i - 1][1]);
        }
        int frmNum = sectorNum * 2352 / frmSize;
        printf("Frames: %5d, Bytes: %9ld\n", frmNum, frmNum * frmSize);

        for (int j = 0; j < frmNum * frmSize; j += 10) {
            fread(&l1, 1, 9, fpImg);
            fwrite(&l1, 1, 9, foutl);
            fread(&r1, 1, 1, fpImg);
            fwrite(&r1, 1, 1, foutr);
        }
        fclose(foutl);

        if (type == 24) {
            xtractColor(&frmSize, &seekPos, &ppmNum);
            allFrmSize += frmSize;
        }
        else if (type == 12) {
            xtractXp(&frmSize, &seekPos, &ppmNum);
            allFrmSize += frmSize;
        }
    }
    fclose(foutr);

    if (!strcmp(arg, "color")) {
        addhColorXp(allFrmSize, 1960, seekPos);
    }
    else if (!strcmp(arg, "xp")) {
        addhColorXp(allFrmSize, 1976, seekPos);
    }
    return 0;
}

int handleBlackAndWhite(FILE* fpImg, long fsize, int lba[MAX_TRACK][MAX_INDEX]) {
    FILE* foutl2 = fopen("input-raw-left2.raw", "wb");
    FILE* foutr2 = fopen("input-raw-right2.raw", "wb");
    if (!foutl2 || !foutr2) {
        if (foutl2) fclose(foutl2);
        if (foutr2) fclose(foutr2);
        return 1;
    }

    unsigned char l1[2] = {};
    unsigned char l2;
    unsigned char r2;
    long frmSize = 11760;
    long ppmNum = 0;

    for (int i = 1; lba[i - 1][1] != -1; i++) {
        FILE* foutl = fopen("input-raw-left.raw", "wb");
        if (!foutl) continue;

        printf("Track[%02d] LBA: %6d, ", i, lba[i - 1][1]);
        off_t offset = (off_t)lba[i - 1][1] * 2352;
        fseeko(fpImg, offset, SEEK_SET);

        unsigned char buf = 0;
        long readsize = 0;
        while (!feof(fpImg)) {
            readsize += (long)fread(&buf, sizeof(unsigned char), 1, fpImg);
            if (isHeader(buf)) {
                fseek(fpImg, -readsize, SEEK_CUR);
                break;
            }
        }

        int sectorNum = 0;
        if (lba[i][0] != -1 && lba[i - 1][1] != -1) {
            sectorNum = lba[i][0] - lba[i - 1][1];
        }
        else if (lba[i][1] != -1 && lba[i - 1][1] != -1) {
            sectorNum = lba[i][1] - lba[i - 1][1];
        }
        if (sectorNum == 0) {
            sectorNum = (int)(fsize / 2352 - lba[i - 1][1]);
        }

        int frmNum = sectorNum * 2352 / frmSize;
        printf("Frames: %5d, Bytes: %9d\n", frmNum, sectorNum * 2352);

        for (int j = 0; j < sectorNum * 2352; j += 4) {
            fread(&l1, 1, 2, fpImg);
            fwrite(&l1, 1, 2, foutl);
            fread(&l2, 1, 1, fpImg);
            fwrite(&l2, 1, 1, foutl2);
            fread(&r2, 1, 1, fpImg);
            fwrite(&r2, 1, 1, foutr2);
        }

        if (lba[i][0] != -1) {
            // skip pregap
            fseeko(fpImg, (off_t)((lba[i][1] - lba[i][0]) * 2352), SEEK_CUR);
        }
        fclose(foutl);
        xtract(&ppmNum);
    }

    fclose(foutl2);
    fclose(foutr2);
    addh();
    unlink("input-raw-left2.raw");
    return 0;
}

void printUsage() {
    printf(
        "VideoNow PVD Disc Decoder for macOS\n"
        "Usage:\n"
        "\t./PVDTools bw <cue file>\n"
        "\t\t Output pgm (40 x 80 gray scale) and wav file\n"
        "\t./PVDTools color <cue file>\n"
        "\t\t Output ppm (144 x 80 full color) and wav file\n"
        "\t./PVDTools xp <cue file>\n"
        "\t\t Output ppm (144 x 80 full color) and wav file\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printUsage();
        return 1;
    }

    // Get directory and filename components
    char dir[MAX_PATH] = {};
    char fname[MAX_PATH] = {};
    char ext[MAX_PATH] = {};
    splitPath(argv[2], dir, fname, ext);

    if (strcmp(ext, ".cue") != 0) {
        printUsage();
        return 1;
    }

    // Process cue file
    FILE* fp = fopen(argv[2], "r");
    if (!fp) {
        printf("Error: Cannot open cue file\n");
        return 1;
    }

    int lba[MAX_TRACK][MAX_INDEX] = {};
    for (int i = 0; i < MAX_TRACK; i++) {
        for (int j = 0; j < MAX_INDEX; j++) {
            lba[i][j] = -1;
        }
    }

    char binPath[MAX_PATH] = {};
    HandleCueFile(fp, fname, lba);
    fclose(fp);

    // Construct full path to binary file
    makePath(binPath, dir, fname, NULL);

    // Open binary file
    int fd = open(binPath, O_RDONLY);
    if (fd == -1) {
        printf("Error: Cannot open binary file\n");
        return 1;
    }

    FILE* fpImg = fdopen(fd, "rb");
    if (!fpImg) {
        close(fd);
        printf("Error: Cannot create file handle\n");
        return 1;
    }

    struct stat stbuf;
    if (fstat(fd, &stbuf) == -1) {
        fclose(fpImg);
        printf("Error: Cannot get file size\n");
        return 1;
    }

    // Process based on command
    int result;
    if (strcmp(argv[1], "bw") == 0) {
        result = handleBlackAndWhite(fpImg, stbuf.st_size, lba);
    }
    else if (strcmp(argv[1], "color") == 0 || strcmp(argv[1], "xp") == 0) {
        result = handleColorOrXp(argv[1], fpImg, stbuf.st_size, lba);
    }
    else {
        printUsage();
        result = 1;
    }

    fclose(fpImg);
    return result;
}

