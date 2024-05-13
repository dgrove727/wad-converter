#include "decompress.h"
#include "common.h"
#include <stdlib.h>

void *decompress(void *data, size_t outputLen)
{
	byte *uncompressed = (byte *)malloc(outputLen + 16);	// Add 16 to make sure we have enough room.

	int readCursor = 0;
	int writeCursor = 0;

	byte *compressed = static_cast<byte *>(data);

	// Continue decompressing until "outputLen" bytes have been written.
	while (writeCursor < outputLen) {
		byte key = compressed[readCursor];
		readCursor++;

		// Read the key bits from LSB to MSB.
		for (int keyCursor = 0; keyCursor < 8; keyCursor++) {
			if (key & 1) {
				// Compressed data.
				short copyCursor = (compressed[readCursor] << 4) | ((compressed[readCursor + 1] >> 4) & 0xF);
				char copyLen = compressed[readCursor + 1] & 0xF;
				readCursor += 2;

				copyCursor = writeCursor - copyCursor;

				// Copy bytes to the current write position which were written previously.
				while (copyLen > 0) {
					uncompressed[writeCursor] = uncompressed[copyCursor];
					writeCursor++;
					copyCursor++;
					copyLen--;
				}
			}
			else {
				// Uncompressed data.
				uncompressed[writeCursor] = compressed[readCursor];
				writeCursor++;
				readCursor++;
			}

			key >>= 1;	// Advance to the next key bit to read.
		}
	}

	return uncompressed;
}
