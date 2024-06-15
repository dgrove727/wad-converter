#pragma once
#include "common.h"

byte *FlatToPNG(const byte *flatData, int width, int height, int *outputLen);
byte *PNGToFlat(byte *pngData, int pngLength, int *width, int *height);

byte *PatchToJagTexture(const byte *patchData, size_t dataLen, int *outputLen);

byte *CropPCPatch(const byte *patchData, size_t dataLen, int *outputLen, byte transparentIndex);

void PCSpriteToJag(const byte *lumpData, int lumpSize, byte *jagHeader, int *jagHeaderLen, byte *jagData, int *jagDataLen);
byte *PatchToPNG(byte *patchData, size_t dataLen, int *outputLen, byte transparentIndex);
byte *JagSpriteToPNG(byte *jagHeader, byte *jagData, size_t headerLen, size_t dataLen, int *outputLen);
byte *RawToPatch(byte *rawImage, int width, int height, int *outputLen, byte transparentIndex);
byte *PNGToPatch(byte *pngData, size_t dataLen, int *outputLen, byte transparentIndex);
