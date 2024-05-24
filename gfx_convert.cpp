#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "gfx_convert.h"

byte *FlatToPNG(byte *flatData, int width, int height, int *outputLen)
{
	return nullptr;
}

byte *PNGToFlat(byte *pngData, int width, int height, int *outputLen)
{
	return nullptr;
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

byte *PNGToPatch(byte *pngData, size_t dataLen, int *outputLen)
{
//	stbi_load_from_memory(pngData, dataLen, x, y, channels_in_file, 1);
	return nullptr;
}
