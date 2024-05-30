#pragma once
#include "common.h"

void ConvertSpriteDataFromPCToJag(byte *lumpData, int lumpSize, byte *jagHeader, int *jagHeaderLen, byte *jagData, int *jagDataLen);
byte *ConvertSpriteDataFromJagToPC(byte *jagHeader, int jagHeaderLen, byte *jagData, int jagDataLen, int *outputLen);