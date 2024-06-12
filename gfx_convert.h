#pragma once
#include "common.h"

byte *FlatToPNG(byte *flatData, int width, int height, int *outputLen);
byte *PNGToFlat(byte *pngData, int pngLength, int *width, int *height);

void PCSpriteToJag(const byte *lumpData, int lumpSize, byte *jagHeader, int *jagHeaderLen, byte *jagData, int *jagDataLen);
byte *PatchToPNG(byte *patchData, size_t dataLen, int *outputLen);
byte *JagSpriteToPNG(byte *jagHeader, byte *jagData, size_t headerLen, size_t dataLen, int *outputLen);
byte *PNGToPatch(byte *pngData, size_t dataLen, int *outputLen);
