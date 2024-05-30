#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "gfx_convert.h"

typedef struct
{
	byte r, g, b;
} palentry_t;

// SRB2 2.0 palette
palentry_t palette[256] = {
	{247, 247, 247},
	{239, 239, 239},
	{231, 231, 231},
	{223, 223, 223},
	{215, 215, 215},
	{207, 207, 207},
	{199, 199, 199},
	{191, 191, 191},
	{183, 183, 183},
	{175, 175, 175},
	{167, 167, 167},
	{159, 159, 159},
	{151, 151, 151},
	{143, 143, 143},
	{135, 135, 135},
	{127, 127, 127},
	{119, 119, 119},
	{111, 111, 111},
	{103, 103, 103},
	{95, 95, 95},
	{87, 87, 87},
	{79, 79, 79},
	{71, 71, 71},
	{63, 63, 63},
	{55, 55, 55},
	{47, 47, 47},
	{39, 39, 39},
	{31, 31, 31},
	{23, 23, 23},
	{15, 15, 15},
	{7, 7, 7},
	{0, 0, 0},
	{191, 167, 143},
	{183, 160, 136},
	{175, 152, 128},
	{167, 144, 120},
	{159, 137, 113},
	{150, 129, 105},
	{142, 121, 97},
	{134, 114, 90},
	{126, 106, 82},
	{117, 98, 74},
	{109, 90, 66},
	{101, 83, 59},
	{93, 75, 51},
	{84, 67, 43},
	{76, 60, 36},
	{67, 51, 27},
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
	{255, 238, 220},
	{255, 220, 185},
	{255, 203, 151},
	{255, 185, 117},
	{255, 168, 85},
	{255, 151, 54},
	{255, 134, 25},
	{255, 117, 0},
	{243, 109, 0},
	{229, 101, 0},
	{216, 93, 0},
	{203, 85, 0},
	{190, 77, 0},
	{177, 69, 0},
	{164, 61, 0},
	{151, 54, 0},
	{255, 255, 239},
	{255, 255, 207},
	{255, 255, 175},
	{255, 255, 143},
	{255, 255, 111},
	{255, 255, 79},
	{255, 255, 47},
	{255, 255, 15},
	{255, 255, 0},
	{207, 207, 0},
	{175, 175, 0},
	{143, 143, 0},
	{111, 111, 0},
	{79, 79, 0},
	{47, 47, 0},
	{15, 15, 0},
	{255, 255, 115},
	{235, 219, 87},
	{215, 187, 67},
	{195, 155, 47},
	{175, 123, 31},
	{155, 91, 19},
	{135, 67, 7},
	{115, 43, 0},
	{255, 255, 255},
	{255, 223, 223},
	{255, 191, 191},
	{255, 159, 159},
	{255, 127, 127},
	{255, 95, 95},
	{255, 63, 63},
	{255, 31, 31},
	{255, 0, 0},
	{239, 0, 0},
	{223, 0, 0},
	{207, 0, 0},
	{191, 0, 0},
	{175, 0, 0},
	{159, 0, 0},
	{143, 0, 0},
	{127, 0, 0},
	{111, 0, 0},
	{95, 0, 0},
	{79, 0, 0},
	{63, 0, 0},
	{47, 0, 0},
	{31, 0, 0},
	{15, 0, 0},
	{255, 183, 183},
	{243, 163, 163},
	{231, 143, 143},
	{219, 123, 123},
	{203, 107, 107},
	{191, 91, 91},
	{179, 79, 79},
	{167, 63, 63},
	{142, 46, 0},
	{134, 39, 0},
	{126, 32, 0},
	{117, 25, 0},
	{109, 18, 0},
	{101, 11, 0},
	{93, 5, 0},
	{85, 0, 0},
	{119, 255, 79},
	{112, 240, 75},
	{105, 224, 70},
	{97, 208, 65},
	{90, 192, 60},
	{82, 176, 55},
	{75, 160, 50},
	{67, 144, 45},
	{60, 128, 40},
	{53, 112, 35},
	{45, 96, 30},
	{38, 80, 25},
	{30, 64, 20},
	{23, 48, 15},
	{15, 32, 10},
	{7, 15, 4},
	{222, 255, 168},
	{199, 228, 148},
	{173, 200, 128},
	{149, 173, 107},
	{124, 146, 88},
	{100, 119, 68},
	{74, 90, 48},
	{50, 63, 29},
	{0, 255, 0},
	{0, 223, 0},
	{0, 191, 0},
	{0, 159, 0},
	{0, 127, 0},
	{0, 95, 0},
	{0, 63, 0},
	{0, 31, 0},
	{255, 111, 255},
	{255, 0, 255},
	{223, 0, 223},
	{191, 0, 191},
	{159, 0, 159},
	{127, 0, 127},
	{95, 0, 95},
	{63, 0, 63},
	{233, 233, 243},
	{196, 196, 225},
	{157, 157, 206},
	{119, 119, 187},
	{84, 84, 167},
	{65, 65, 131},
	{46, 46, 92},
	{27, 27, 52},
	{213, 241, 255},
	{191, 235, 255},
	{170, 227, 255},
	{149, 221, 255},
	{128, 214, 255},
	{106, 207, 255},
	{85, 200, 255},
	{63, 191, 255},
	{55, 157, 223},
	{47, 143, 191},
	{39, 119, 159},
	{31, 95, 127},
	{0, 191, 191},
	{0, 127, 127},
	{0, 95, 95},
	{0, 63, 63},
	{239, 239, 255},
	{207, 207, 255},
	{175, 175, 255},
	{143, 143, 255},
	{111, 111, 255},
	{79, 79, 255},
	{47, 47, 255},
	{15, 15, 255},
	{0, 0, 255},
	{0, 0, 223},
	{0, 0, 207},
	{0, 0, 191},
	{0, 0, 175},
	{0, 0, 159},
	{0, 0, 143},
	{0, 0, 127},
	{0, 0, 111},
	{0, 0, 95},
	{0, 0, 79},
	{0, 0, 63},
	{0, 0, 47},
	{0, 0, 31},
	{0, 0, 15},
	{0, 255, 255},
	{207, 127, 207},
	{183, 111, 183},
	{159, 95, 159},
	{135, 79, 135},
	{111, 63, 111},
	{87, 47, 87},
	{63, 31, 63},
	{39, 15, 39},
};

/*
* Doom palette
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
};*/

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
	int closestDist = -1;
	for (int i = 0; i < 256; i++)
	{
		const palentry_t *palEntry = &palette[i];

		int dist_r = abs(palEntry->r - r);
		int dist_g = abs(palEntry->g - g);
		int dist_b = abs(palEntry->b - b);

		int dist = (dist_r * dist_r) + (dist_g * dist_g) + (dist_b * dist_b);

		if (closestDist < 0 || dist < closestDist)
		{
			closestIndex = i;
			closestDist = dist;
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
	byte data[256];
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
/*
byte *JagSpriteToPNG(const byte *sprHeader, const byte *sprData, size_t headerLen, size_t dataLen, int *outputLen)
{
	// TODO: Not finished/tested yet.

	const patchHeader_t *header = (const patchHeader_t *)sprHeader;

	byte *rawImage = (byte *)malloc(header->width * header->height * 1);
	memset(rawImage, 247, header->width * header->height * 1); // Transparent value

	// For jaguar, column offsets are 16-bit unsigned
	unsigned short *colOfsPointer = (unsigned short*)header->columnofs;
	unsigned short *columnOfs = (unsigned short *)malloc(header->width * sizeof(unsigned short));
	for (int w = 0; w < header->width; w++)
		columnOfs[w] = *colOfsPointer++;

	// Reading pixel data
	for (int w = 0; w < header->width; w++)
	{
		int top;
		int post_p = columnOfs[w];

		while ((top = sprHeader[post_p]) != 0xff)
		{
			int len = sprHeader[post_p + 1];
			unsigned short pixel_p = sprHeader[post_p + 2];
			
			for (int p = 0; p < len; p++)
			{
				size_t pos = w + header->width * (top + p);
				rawImage[pos] = sprData[pixel_p + p];
			}
			post_p += 4;
		}
	}

	return stbi_write_png_to_mem(rawImage, 0, header->width, header->height, 1, outputLen);
}
*/
byte *PNGToPatch(byte *pngData, size_t dataLen, int *outputLen)
{
	const byte transparentIndex = 247;
	int width, height;
	byte *indexedImage = PNGToFlat(pngData, dataLen, &width, &height);

	// Modern memory is cheap, so let's just allocate 1mb as workspace.
	byte *postData = (byte *)malloc(1 * 1024 * 1024);
	size_t postDataSize = 0;

	patchHeader_t header;
	header.leftoffset = 0x23;
	header.topoffset = 0x3b;
	header.width = (unsigned short)width;
	header.height = (unsigned short)height;

	unsigned int columnOfs[4096]; // Again, because memory is cheap...
	size_t numColumnOfs = 0;
	unsigned int nextAvailableColOf = 0; // Not going to know this start point until we finish (dependent on the # of posts we end up with)

	post_t post;
	post.unused = 0;

	for (int x = 0; x < width; x++)
	{
		int y = 0;
		bool lookingForColStart = true;

		bool wroteNothing = true;

		int colLength = 0;
		while (y < height)
		{
			// Get pixel value
			size_t pixelLocation = (y * width) + x;
			byte pixel = indexedImage[pixelLocation];

			if (pixel == transparentIndex && !lookingForColStart)
			{
				lookingForColStart = true;

				// Flush post data
				postData[postDataSize++] = post.topdelta;
				postData[postDataSize++] = post.length;
				postData[postDataSize++] = post.unused;
				for (int i = 0; i < post.length; i++)
					postData[postDataSize++] = post.data[i];

				postData[postDataSize++] = post.data[post.length - 1]; // dummy byte
				colLength += post.length + 4;
			}
			else if (pixel != transparentIndex && lookingForColStart)
			{
				wroteNothing = false;

				post.topdelta = y;
				post.length = 0;
				post.unused = pixel;
				lookingForColStart = false;
				post.data[post.length++] = pixel;
			}
			else if (pixel != transparentIndex && !lookingForColStart)
			{
				// Add an additional pixel to this post
				post.data[post.length++] = pixel;
			}

			y++;
		}

		if (wroteNothing)
		{
			postData[postDataSize++] = 255; // End marker

			columnOfs[numColumnOfs++] = nextAvailableColOf;
			nextAvailableColOf += 1;
		}
		else if (!lookingForColStart) // Instead of hitting a transparent pixel to end this post, we hit the literal bottom of the image.
		{
			// Flush post data
			postData[postDataSize++] = post.topdelta;
			postData[postDataSize++] = post.length;
			postData[postDataSize++] = post.unused;
			for (int i = 0; i < post.length; i++)
				postData[postDataSize++] = post.data[i];

			postData[postDataSize++] = post.data[post.length - 1]; // dummy byte

			postData[postDataSize++] = 255; // End marker

			colLength += post.length + 5;

			columnOfs[numColumnOfs++] = nextAvailableColOf;
			nextAvailableColOf += colLength;
		}
		else
		{
			postData[postDataSize++] = 255; // End marker

			colLength++;

			columnOfs[numColumnOfs++] = nextAvailableColOf;
			nextAvailableColOf += colLength;
		}
	}

	// Now put it all together
	size_t lumpSize = 8 + (numColumnOfs * 4) + (postDataSize);
	byte *patchImage = (byte *)malloc(lumpSize);

	byte *cursor = patchImage;
	// Write the header
	memcpy(cursor, &header, 8); // Only first 8 bytes of header
	cursor += 8;

	// translate the columnofs positions now that we know how big columnofs is going to be
	for (int i = 0; i < numColumnOfs; i++)
		columnOfs[i] += 8 + (numColumnOfs * 4);

	// Write the columnofs information
	memcpy(cursor, columnOfs, sizeof(unsigned int) * numColumnOfs);
	cursor += sizeof(unsigned int) * numColumnOfs;

	// Finally, write the column post data
	memcpy(cursor, postData, postDataSize);

	// Cleanup
	free(postData);

	*outputLen = lumpSize;

	return patchImage;
}
/*
void *PNGToJagSprite(byte *pngData, size_t pngLen, byte *sprHeader, int *headerLen, byte *sprData, int *dataLen)
{
	return nullptr;
}
*/