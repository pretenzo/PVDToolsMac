// cuefileParser.h
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRACK 100
#define MAX_INDEX 100

int MSFtoLBA(
    unsigned char byMinute,
    unsigned char bySecond,
    unsigned char byFrame
);

bool HandleCueFile(FILE* fp, char* fname, int lba[MAX_TRACK][MAX_INDEX]);
