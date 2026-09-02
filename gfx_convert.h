#pragma once
#include "common.h"

uint8_t *FlatToPNG(const uint8_t *flatData, int32_t width, int32_t height, int32_t *outputLen);
uint8_t *PNGToFlat(const uint8_t *pngData, int32_t pngLength, int32_t *width, int32_t *height);
uint8_t *RawToPNG(const uint8_t *rawData, int32_t width, int32_t height, int32_t *outputLen);

uint8_t *FlatMipmaps(const uint8_t *data, int dataLen, int numlevels, int *outputLen);
uint8_t *PatchMipmaps(const uint8_t *data, int width, int height, int numlevels, int *outputLen);

bool ContainsPixel(const uint8_t *rawImage, uint16_t width, uint16_t height, uint8_t index);
uint8_t *RawToJagTexture(const uint8_t *rawImage, uint16_t width, uint16_t height);
uint8_t *PatchToJagTexture(const uint8_t *patchData, size_t dataLen, int32_t *outputLen);

uint8_t *CropPCPatch(const uint8_t *patchData, size_t dataLen, int32_t *outputLen, uint8_t transparentIndex);

void PCSpriteToJag(const uint8_t *lumpData, int32_t lumpSize, uint8_t *jagHeader, int32_t *jagHeaderLen, uint8_t *jagData, int32_t *jagDataLen);
void PCSpriteToJagNarrow(const uint8_t *lumpData, int32_t lumpSize, uint8_t *jagHeader, int32_t *jagHeaderLen, uint8_t *jagData, int32_t *jagDataLen);
uint8_t *PatchToPNG(const uint8_t *patchData, size_t dataLen, int32_t *outputLen, uint8_t transparentIndex);
uint8_t *JagSpriteToPNG(uint8_t *jagHeader, uint8_t *jagData, size_t headerLen, size_t dataLen, int32_t *outputLen);
uint8_t *PatchToRaw(const uint8_t *patchData, size_t dataLen, int32_t *outputLen, uint8_t transparentIndex);
uint8_t *RawToPatch(uint8_t *rawImage, int32_t width, int32_t height, int32_t *outputLen, uint8_t transparentIndex);
uint8_t *PNGToPatch(uint8_t *pngData, size_t dataLen, int32_t *outputLen, uint8_t transparentIndex);

typedef struct
{
	uint8_t *data;
	size_t dataSize;
} pngresult_t;

pngresult_t PNGTo15Bit(const uint8_t *pngData, size_t dataLen);