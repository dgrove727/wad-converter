#pragma once
#include "common.h"

byte *FlatToPNG(const byte *flatData, int32_t width, int32_t height, int32_t *outputLen);
byte *PNGToFlat(byte *pngData, int32_t pngLength, int32_t *width, int32_t *height);
byte *RawToPNG(const byte *rawData, int32_t width, int32_t height, int32_t *outputLen);

byte *FlatMipmaps(const byte *data, int dataLen, int numlevels, int *outputLen);
byte *PatchMipmaps(const byte *data, int width, int height, int numlevels, int *outputLen);

bool ContainsPixel(const byte *rawImage, uint16_t width, uint16_t height, byte index);
byte *RawToJagTexture(const byte *rawImage, uint16_t width, uint16_t height);
byte *PatchToJagTexture(const byte *patchData, size_t dataLen, int32_t *outputLen);

byte *CropPCPatch(const byte *patchData, size_t dataLen, int32_t *outputLen, byte transparentIndex);

void PCSpriteToJag(const byte *lumpData, int32_t lumpSize, byte *jagHeader, int32_t *jagHeaderLen, byte *jagData, int32_t *jagDataLen);
byte *PatchToPNG(const byte *patchData, size_t dataLen, int32_t *outputLen, byte transparentIndex);
byte *JagSpriteToPNG(byte *jagHeader, byte *jagData, size_t headerLen, size_t dataLen, int32_t *outputLen);
byte *PatchToRaw(const byte *patchData, size_t dataLen, int32_t *outputLen, byte transparentIndex);
byte *RawToPatch(byte *rawImage, int32_t width, int32_t height, int32_t *outputLen, byte transparentIndex);
byte *PNGToPatch(byte *pngData, size_t dataLen, int32_t *outputLen, byte transparentIndex);
