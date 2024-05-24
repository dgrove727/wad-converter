#pragma once
#include "common.h"

byte *FlatToPNG(byte *flatData, int width, int height, int *outputLen);
byte *PNGToFlat(byte *pngData, int width, int height, int *outputLen);

byte *PatchToPNG(byte *patchData, size_t dataLen, int *outputLen);
byte *PNGToPatch(byte *pngData, size_t dataLen, int *outputLen);