#include <cstdio>
#include <cstdlib>

/*
Table of contents entry format:
	char name[8]	// When compressed, MSB of first byte is set.
	long pointer
	long size		// Uncompressed size goes here.
*/

/*
Read bit for key

If bit is 1 {
	Read 16-bit word (big-endian)
		Format: LLLLLLLL LLLLCCCC
			L = location
			C = count

	Copy "count" bytes from output located at
		"location" bytes behind current position
}
Else {
	Copy next byte from key
}
*/

void *decompress(void *data, size_t outputLen);


int main() {
	// Read in all the bytes found in SIDEDEFS.LMP.
	FILE *inputFile = fopen("sidedefs.lmp", "rb");

	fseek(inputFile, 0, SEEK_END);
	size_t inputLen = ftell(inputFile);
	rewind(inputFile);

	unsigned char inputData[0x10000];	// No idea how big the compressed data is.
	fread(inputData, 1, inputLen, inputFile);

	fclose(inputFile);


	// Decompress the data.

	size_t outputLen = 0x43F8;	// Hard-coded uncompressed size found in WAD table of contents.
	void *outputData = decompress(inputData, outputLen);


	// Write out the decompressed data to OUTPUT.

	FILE *outputFile = fopen("output", "wb");

	fwrite(outputData, 1, outputLen, outputFile);

	fclose(outputFile);


	// Free up memory.

	free(outputData);
}

void *decompress(void *data, size_t outputLen)
{
	unsigned char *uncompressed = (unsigned char *)malloc(outputLen + 16);	// Add 16 to make sure we have enough room.

	int readCursor = 0;
	int writeCursor = 0;

	unsigned char *compressed = static_cast<unsigned char*>(data);

	// Continue decompressing until "outputLen" bytes have been written.
	while (writeCursor < outputLen) {
		unsigned char key = compressed[readCursor];
		readCursor++;

		// Read the key bytes from LSB to MSB.
		for (int keyCursor = 0; keyCursor < 8; keyCursor++) {
			if (key & 1) {
				// Compressed data.
				short copyCursor = (compressed[readCursor] << 4) | ((compressed[readCursor+1] >> 4) & 0xF);
				char copyLen = compressed[readCursor+1] & 0xF;
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
