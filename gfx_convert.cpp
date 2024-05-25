#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "gfx_convert.h"

typedef struct
{
	byte r, g, b;
} palentry_t;

palentry_t palette[256] = {
	{0, 0, 0},
	{31, 23, 11},
	{23, 15, 7},
	{75, 75, 75},
	{255, 255, 255},
	{27, 27, 27},
	{19, 19, 19},
	{11, 11, 11},
	{7, 7, 7},
	{47, 55, 31},
	{35, 43, 15},
	{23, 31, 7},
	{15, 23, 0},
	{79, 59, 43},
	{71, 51, 35},
	{63, 43, 27},
	{255, 183, 183},
	{247, 171, 171},
	{243, 163, 163},
	{235, 151, 151},
	{231, 143, 143},
	{223, 135, 135},
	{219, 123, 123},
	{211, 115, 115},
	{203, 107, 107},
	{199, 99, 99},
	{191, 91, 91},
	{187, 87, 87},
	{179, 79, 79},
	{175, 71, 71},
	{167, 63, 63},
	{163, 59, 59},
	{155, 51, 51},
	{151, 47, 47},
	{143, 43, 43},
	{139, 35, 35},
	{131, 31, 31},
	{127, 27, 27},
	{119, 23, 23},
	{115, 19, 19},
	{107, 15, 15},
	{103, 11, 11},
	{95, 7, 7},
	{91, 7, 7},
	{83, 7, 7},
	{79, 0, 0},
	{71, 0, 0},
	{67, 0, 0},
	{255, 235, 223},
	{255, 227, 211},
	{255, 219, 199},
	{255, 211, 187},
	{255, 207, 179},
	{255, 199, 167},
	{255, 191, 155},
	{255, 187, 147},
	{255, 179, 131},
	{247, 171, 123},
	{239, 163, 115},
	{231, 155, 107},
	{223, 147, 99},
	{215, 139, 91},
	{207, 131, 83},
	{203, 127, 79},
	{191, 123, 75},
	{179, 115, 71},
	{171, 111, 67},
	{163, 107, 63},
	{155, 99, 59},
	{143, 95, 55},
	{135, 87, 51},
	{127, 83, 47},
	{119, 79, 43},
	{107, 71, 39},
	{95, 67, 35},
	{83, 63, 31},
	{75, 55, 27},
	{63, 47, 23},
	{51, 43, 19},
	{43, 35, 15},
	{239, 239, 239},
	{231, 231, 231},
	{223, 223, 223},
	{219, 219, 219},
	{211, 211, 211},
	{203, 203, 203},
	{199, 199, 199},
	{191, 191, 191},
	{183, 183, 183},
	{179, 179, 179},
	{171, 171, 171},
	{167, 167, 167},
	{159, 159, 159},
	{151, 151, 151},
	{147, 147, 147},
	{139, 139, 139},
	{131, 131, 131},
	{127, 127, 127},
	{119, 119, 119},
	{111, 111, 111},
	{107, 107, 107},
	{99, 99, 99},
	{91, 91, 91},
	{87, 87, 87},
	{79, 79, 79},
	{71, 71, 71},
	{67, 67, 67},
	{59, 59, 59},
	{55, 55, 55},
	{47, 47, 47},
	{39, 39, 39},
	{35, 35, 35},
	{119, 255, 111},
	{111, 239, 103},
	{103, 223, 95},
	{95, 207, 87},
	{91, 191, 79},
	{83, 175, 71},
	{75, 159, 63},
	{67, 147, 55},
	{63, 131, 47},
	{55, 115, 43},
	{47, 99, 35},
	{39, 83, 27},
	{31, 67, 23},
	{23, 51, 15},
	{19, 35, 11},
	{11, 23, 7},
	{191, 167, 143},
	{183, 159, 135},
	{175, 151, 127},
	{167, 143, 119},
	{159, 135, 111},
	{155, 127, 107},
	{147, 123, 99},
	{139, 115, 91},
	{131, 107, 87},
	{123, 99, 79},
	{119, 95, 75},
	{111, 87, 67},
	{103, 83, 63},
	{95, 75, 55},
	{87, 67, 51},
	{83, 63, 47},
	{159, 131, 99},
	{143, 119, 83},
	{131, 107, 75},
	{119, 95, 63},
	{103, 83, 51},
	{91, 71, 43},
	{79, 59, 35},
	{67, 51, 27},
	{123, 127, 99},
	{111, 115, 87},
	{103, 107, 79},
	{91, 99, 71},
	{83, 87, 59},
	{71, 79, 51},
	{63, 71, 43},
	{55, 63, 39},
	{255, 255, 115},
	{235, 219, 87},
	{215, 187, 67},
	{195, 155, 47},
	{175, 123, 31},
	{155, 91, 19},
	{135, 67, 7},
	{115, 43, 0},
	{255, 255, 255},
	{255, 219, 219},
	{255, 187, 187},
	{255, 155, 155},
	{255, 123, 123},
	{255, 95, 95},
	{255, 63, 63},
	{255, 31, 31},
	{255, 0, 0},
	{239, 0, 0},
	{227, 0, 0},
	{215, 0, 0},
	{203, 0, 0},
	{191, 0, 0},
	{179, 0, 0},
	{167, 0, 0},
	{155, 0, 0},
	{139, 0, 0},
	{127, 0, 0},
	{115, 0, 0},
	{103, 0, 0},
	{91, 0, 0},
	{79, 0, 0},
	{67, 0, 0},
	{231, 231, 255},
	{199, 199, 255},
	{171, 171, 255},
	{143, 143, 255},
	{115, 115, 255},
	{83, 83, 255},
	{55, 55, 255},
	{27, 27, 255},
	{0, 0, 255},
	{0, 0, 227},
	{0, 0, 203},
	{0, 0, 179},
	{0, 0, 155},
	{0, 0, 131},
	{0, 0, 107},
	{0, 0, 83},
	{255, 255, 255},
	{255, 235, 219},
	{255, 215, 187},
	{255, 199, 155},
	{255, 179, 123},
	{255, 163, 91},
	{255, 143, 59},
	{255, 127, 27},
	{243, 115, 23},
	{235, 111, 15},
	{223, 103, 15},
	{215, 95, 11},
	{203, 87, 7},
	{195, 79, 0},
	{183, 71, 0},
	{175, 67, 0},
	{255, 255, 255},
	{255, 255, 215},
	{255, 255, 179},
	{255, 255, 143},
	{255, 255, 107},
	{255, 255, 71},
	{255, 255, 35},
	{255, 255, 0},
	{167, 63, 0},
	{159, 55, 0},
	{147, 47, 0},
	{135, 35, 0},
	{79, 59, 39},
	{67, 47, 27},
	{55, 35, 19},
	{47, 27, 11},
	{0, 0, 83},
	{0, 0, 71},
	{0, 0, 59},
	{0, 0, 47},
	{0, 0, 35},
	{0, 0, 23},
	{0, 0, 11},
	{0, 255, 255},
	{255, 159, 67},
	{255, 231, 75},
	{255, 123, 255},
	{255, 0, 255},
	{207, 0, 207},
	{159, 0, 155},
	{111, 0, 107},
	{167, 107, 107},
};

byte GetIndexFromRGB(byte r, byte g, byte b)
{
	for (int i = 0; i < 256; i++)
	{
		const palentry_t *palEntry = &palette[i];

		if (palEntry->r == r && palEntry->g == g && palEntry->b == b)
			return (byte)i;
	}

	// If we got here, no entry matched. Try to find the closest match.

	int closestIndex = 0;
	int closestDiff = -1;
	for (int i = 0; i < 256; i++)
	{
		const palentry_t *palEntry = &palette[i];

		int diff = abs(palEntry->r - r) + abs(palEntry->g - g) + abs(palEntry->b - b);

		if (closestDiff < 0 || diff < closestDiff)
		{
			closestIndex = i;
			closestDiff = diff;
		}
	}

	return (byte)closestIndex;
}

byte *RGBToIndexed(byte *rgbData, int width, int height)
{
	byte *indexedImage = (byte *)malloc(width * height);
	int z = 0;
	for (int i = 0; i < width * height * 3; i += 3)
	{
		byte r = rgbData[i];
		byte g = rgbData[i + 1];
		byte b = rgbData[i + 2];

		byte palIndex = GetIndexFromRGB(r, g, b);

		indexedImage[z++] = palIndex;
	}

	return indexedImage;
}

byte *FlatToPNG(byte *flatData, int width, int height, int *outputLen)
{
	return stbi_write_png_to_mem(flatData, 0, width, height, 1, outputLen);
}

byte *PNGToFlat(byte *pngData, int pngLength, int *width, int *height)
{
	int channels;

	byte *rawImage = stbi_load_from_memory(pngData, pngLength, width, height, &channels, 3);
	byte *indexedImage = RGBToIndexed(rawImage, *width, *height);

	free(rawImage);

	return indexedImage;
}

// Doom GFX has a header, and then pieces of data called
// 'posts', which are offset (topdelta) from the TOP of the image (ALWAYS, even if it's a mid-column post!)
// are stored in the rest of the file, with a byte marker separating each one to indicate
// if a new row should be started or not
typedef struct
{
	unsigned short width;
	unsigned short height;
	short leftoffset;
	short topoffset;
	unsigned int columnofs[8];
} patchHeader_t;

typedef struct
{
	byte topdelta;
	byte length;
	byte unused;
	byte data[1];
} post_t;

// Works for patches, sprites, all of the transparency-format Doom graphics
// Returns an allocated representation of the 8-bit PNG data (albeit without palette information).
// Up to you to manage the memory lifetime of it!
byte *PatchToPNG(byte *patchData, size_t dataLen, int *outputLen)
{
	patchHeader_t *header = (patchHeader_t *)patchData;

	byte *rawImage = (byte *)malloc(header->width * header->height * 1);
	memset(rawImage, 247, header->width * header->height * 1); // Transparent value

	for (int i = 0; i < header->width; i++)
	{
		unsigned int colOffset = header->columnofs[i];
		const post_t *post = (post_t *)(patchData + colOffset);

		int yPos = 0;
		while (post->topdelta != 255)
		{
			yPos = post->topdelta;
			const byte *pixel = post->data;

			for (int j = 0; j < post->length; j++)
			{
				size_t pixelLocation = (yPos * header->width) + i;

				rawImage[pixelLocation] = *pixel;
				pixel++;
				yPos++;
			}
			
			pixel++; // dummy value
			post = (const post_t *)pixel;
		}
	}

	return stbi_write_png_to_mem(rawImage, 0, header->width, header->height, 1, outputLen);
}

byte *JagSpriteToPNG(byte *sprHeader, byte *sprData, size_t headerLen, size_t dataLen, int *outputLen)
{
	// TODO: Not finished/tested yet.

	patchHeader_t *header = (patchHeader_t *)sprHeader;

	byte *rawImage = (byte *)malloc(header->width * header->height * 1);
	memset(rawImage, 247, header->width * header->height * 1); // Transparent value

	for (int i = 0; i < header->width; i++)
	{
		unsigned int colOffset = header->columnofs[i];
		const post_t *post = (post_t *)(sprData + colOffset);

		int yPos = 0;
		while (post->topdelta != 255)
		{
			yPos = post->topdelta;
			const byte *pixel = post->data;

			for (int j = 0; j < post->length; j++)
			{
				size_t pixelLocation = (yPos * header->width) + i;

				rawImage[pixelLocation] = *pixel;
				pixel++;
				yPos++;
			}

			pixel++; // dummy value
			post = (const post_t *)pixel;
		}
	}

	return stbi_write_png_to_mem(rawImage, 0, header->width, header->height, 1, outputLen);
}

byte *PNGToPatch(byte *pngData, size_t dataLen, int *outputLen)
{
	int width, height;
	byte *indexedImage = PNGToFlat(pngData, dataLen, &width, &height);

	// Modern memory is cheap, so let's just allocate 1mb as workspace.
	byte *postData = (byte *)malloc(1 * 1024 * 1024);
	size_t postDataSize = 0;

	patchHeader_t header;
	header.leftoffset = 0;
	header.topoffset = 0;
	header.width = (unsigned short)width;
	header.height = (unsigned short)height;

	unsigned int columnOfs[4096]; // Again, because memory is cheap...
	size_t numColumnOfs = 0;
	unsigned int nextAvailableColOf = sizeof(header);

	post_t *post;


	// Magic happens here
	// TODO: The actual magic


	// Now put it all together
	size_t lumpSize = sizeof(patchHeader_t) + (numColumnOfs * 4) + (postDataSize);
	byte *patchImage = (byte *)malloc(lumpSize);

	byte *cursor = patchImage;
	// Write the header
	memcpy(cursor, &header, sizeof(patchHeader_t));
	cursor += sizeof(patchHeader_t);

	// Write the columnofs information
	memcpy(cursor, columnOfs, sizeof(unsigned int) * numColumnOfs);
	cursor += sizeof(unsigned int) * numColumnOfs;

	// Finally, write the column post data
	memcpy(cursor, postData, postDataSize);

	// Cleanup
	free(postData);

	return patchImage;
}

void *PNGToJagSprite(byte *pngData, size_t pngLen, byte *sprHeader, int *headerLen, byte *sprData, int *dataLen)
{
	return nullptr;
}
