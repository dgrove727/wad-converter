#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "WADEntry.h"
#include "decompress.h"
#include "gfx_convert.h"

#define		VERSION			1.10
#define		WAD_FORMAT		1

// WAD_FORMAT:
// 0 -- (versions 0.99 - 1.00)
// 1 -- (current)
//
// _32X_ lump doesn't have any info in versions prior to v1.10. When _32X_
// doesn't exist or is empty, we assume the format is 0.

// Use this define to make 32X WAD files viewable in WAD file editors
//#define		FORCE_LITTLE_ENDIAN

// Use this define to make importer create a stand-alone 32X WAD
#define		IMPORT_MODE_WAD

// Use this define when exporting a Jaguar WAD file (not finished)
#define		WADTYPE_JAG

// Temporary
void ReadPC();
void ReadMars();
void DumpData(const char* filename, const byte* data, size_t length)
{
	char path[2048];
	sprintf(path, "./dump/%s", filename);

	FILE* f = fopen(path, "wb");
	fwrite(data, length, 1, f);
	fclose(f);
}

// List that holds all of the extracted WAD entries in RAM.
// The data from a PC or Jag/32X WAD should be extracted into this list first, then
// the export routines will read the WAD entries from this list and write them back out.
// This will simplify the import/export process and not require multiple file handles to
// be open at the same time, as well as avoid a lot of crazy pointer management.
WADEntry *wadEntries = NULL;

int lump_count;
int table_ptr;
int file_size;
int iwad_ptr;

// -- dirty fix --
// corrects lump numbers in command
// window for PNAMES and lumps after it
int lump_value_fix = 0;

int out_file_size = 0xC;
int out_table_size = 0;
int out_table_ptr = 0;	// used temporarily
int out_lump_count;

/////////////////////////////////////////
// These are used when doing PC_2_MARS //
int num_of_textures = 0;
byte *out_texture1_data = 0;
byte *out_texture1_table = 0;
/////////////////////////////////////////

byte *data;
byte *table;

byte *out_table;

byte *pnames_table = 0;
int patches_count;

int texture1_ptr;
int texture1_size;

byte *texture1;

char first_sprite = 1;
char first_texture = 1;
char first_floor = 1;

char texture_end_written = 0;
char floor_end_written = 0;
char sprite_end_written = 0;

int d = 0;
int t = 0;

byte format = 0;

#ifndef IMPORT_MODE_WAD
unsigned short checksum;
#endif

FILE *in_file = 0;
FILE *out_file = 0;

enum
{
	//					32X							JAG
	THINGS,		// 0	compressed					compressed
	LINEDEFS,	// 1	compressed					compressed
	SIDEDEFS,	// 2	compressed					compressed
	VERTEXES,	// 3	big-endian, long words		compressed
	SEGS,		// 4	compressed					compressed
	SSECTORS,	// 5	compressed					compressed
	NODES,		// 6	big-endian, long words		compressed
	SECTORS,	// 7	compressed					compressed
	REJECT,		// 8	(no differences)			compressed
	BLOCKMAP	// 9	big-endian					compressed
};

void Create_32X_();		// 32X conversion info lump
void Read_32X_(int lump);

void ConvertMapData(int lump, char mapfile);
void ConvertMapData32X(int lump, char mapfile);

void ConvertCOLORMAPData(int lump);
void ConvertCOLORMAPData32X(int lump);
void ConvertPLAYPALData(int lump);
void ConvertTEXTURE1Data(int lump);

void WritePatchName(char *name);
void CreatePNAMES(int lump);
void CreateTEXTURE1(int lump);

void ConvertRawData(int lump);
void ConvertFloorData(int lump);
void ConvertTextureData(int lump);
void ConvertTextureData32X(int lump);
void WriteTable(int lump, int ptr, int size);
void WriteTableCustom(int ptr, int size, const char *name);
void WriteTableStart(char type);
void WriteTableEnd(char type);
void WriteTexture1(short x_size, short y_size, const char *name);
int swap_endian(unsigned int i);

enum
{
	TYPE_UNDEFINED,
	TYPE_TEXTURE,
	TYPE_FLOOR,
	TYPE_SPRITE,
};

char type = TYPE_UNDEFINED;

void PC2Mars();
void Mars2PC();

enum
{
	OUTPUT_WAD,
	OUTPUT_BIN,
};

enum
{
	MARS_2_PC,
	PC_2_MARS,
};

char conversion_task;

void Shutdown();

bool SetEntryName(char *entryName, const char *data)
{
	strncpy(entryName, data, 8);
	entryName[8] = '\0';

	bool isCompressed = (entryName[0] >> 7) != 0;
	entryName[0] = entryName[0] & 127;

	return isCompressed;
}

static byte *ReadAllBytes(FILE *f, int *file_size)
{
	fseek(f, 0, SEEK_END);
	*file_size = (int)ftell(f);
	rewind(f);

	byte *buffer = (byte *)malloc(*file_size);
	fread(buffer, *file_size, 1, f);

	return buffer;
}

void GfxTests()
{
	FILE *lump;
	int lumpSize;
	lump = fopen("PLAYA1.png", "rb");
	byte *origPng = ReadAllBytes(lump, &lumpSize);
	fclose(lump);

	// Convert it to a lump
	int outputLen;
	byte *convPatch;
	convPatch = PNGToPatch(origPng, lumpSize, &outputLen);

	// Dump it to disk for SCIENCE
	lump = fopen("PLAYA1_output.lmp", "wb");
	fwrite(convPatch, outputLen, 1, lump);
	fclose(lump);

	// Convert it back to a PNG
	convPatch = PatchToPNG(convPatch, outputLen, &outputLen);

	// Dump PNG to disk for SCIENCE
	lump = fopen("PLAYA1_final.png", "wb");
	fwrite(convPatch, outputLen, 1, lump);
	fclose(lump);
}

int main(int argc, char *argv[])
{
	printf(
		"---------------------------------\n"
		" DOOM 32X / JAGUAR WAD CONVERTER v%01.02f\n"
		" Written by Damian Grove\n"
		" %s\n"
		"---------------------------------\n"
		"\n", VERSION, __DATE__
	);

	// TODO: Add a parameter here that sets Jaguar or 32X mode

	if (argc < 4)
	{
		printf(
			"USAGE: wad32x.exe [command] [in file] [out file]\n"
			"  example 1:  wad32x.exe -export doom32x.bin doompc.wad\n"
			"  example 2:  wad32x.exe -import doompc.wad doom32x.bin\n\n"
		);
		system("PAUSE");
		Shutdown();
	}

	int i = strlen(argv[1]) - 1;

	while (i > 0)
	{
		if(argv[1][i] >= 'A' && argv[1][i] <= 'Z')
			argv[1][i] += 0x20;
		i--;
	}

	if (strcmp(argv[1], "-export") == 0)
	{
		in_file = fopen(argv[2], "rb");
		if (!in_file)
		{
			printf("ERROR: ROM file not found.\n");
			Shutdown();
		}

		out_file = fopen(argv[3], "wb");
		if(!out_file)
		{
			printf("ERROR: Unable to create output file.\n");
			fclose(in_file);
			Shutdown();
		}

		ReadMars();
//		Mars2PC();
	}
	else if (strcmp(argv[1], "-import") == 0)
	{
		in_file = fopen(argv[2], "rb");
		if(!in_file)
		{
			printf("ERROR: WAD file not found.\n");
			Shutdown();
		}

#ifdef IMPORT_MODE_WAD
		out_file = fopen(argv[3], "wb");	// USE THIS FOR OUTPUT_WAD
		if(!out_file){
			printf("ERROR: Unable to create output file.\n");
			fclose(in_file);
			Shutdown();
		}
#else
		out_file = fopen(argv[3], "r+b");	// USE THIS FOR OUTPUT_BIN
		if(!out_file)
		{
			printf("ERROR: Unable to open output file.\n");
			fclose(in_file);
			Shutdown();
		}
#endif

		ReadPC();
//		PC2Mars();
	}
	else
	{
		printf("ERROR: Invalid command.\n");
		Shutdown();
	}

	return 0;
}

void Shutdown()
{
	Listable::RemoveAll((Listable **)&wadEntries);

	if (data)
		free(data);
	if (table)
		free(table);
	if (texture1)
		free(texture1);
	if (out_texture1_data)
		free(out_texture1_data);
	if (out_texture1_table)
		free(out_texture1_table);
	if (pnames_table)
		free(pnames_table);
	if (out_table)
		free(out_table);
	if (in_file)
		fclose(in_file);
	if (out_file)
		fclose(out_file);

	exit(0);
}

const char *exclusionList[] = {
	"P1_START",
	"P1_END",
	"P2_START",
	"P2_END",
	"F1_START",
	"F1_END",
	"F2_START",
	"F2_END",
	"TEXTURE1",
	"TEXTURE2",
	"PNAMES",
	"DEMO1",
	"DEMO2",
	"DEMO3",
	"DEMO4",
	"ENDOOM",
	"GENMIDI",
	"DMXGUS",
	NULL,
};

bool IsInExclusionList(const char *entryName)
{
	for (int i = 0; exclusionList[i] != NULL; i++)
	{
		if (!strcmp(entryName, exclusionList[i]))
			return true;
	}

	return false;
}

void ReadMars()
{
	fseek(in_file, 0, SEEK_END);
	file_size = ftell(in_file);
	rewind(in_file);

	char label[5];
	label[4] = '\0';

	// Read the file signature to verify the supplied file is an IWAD or PWAD.
	fread(label, 4, 1, in_file);
	if (!strcmp(label, "IWAD") || !strcmp(label, "PWAD"))
	{
		// "DAWI" or "DAWP" found
		fread(&lump_count, 4, 1, in_file);
		lump_count = swap_endian(lump_count);
		if (lump_count > 0 && lump_count <= 2048)
		{
			// limit based on "MAXLUMPS" in Jaguar source
			fread(&table_ptr, 4, 1, in_file);
			table_ptr = swap_endian(table_ptr);
			if (table_ptr >= 0xC && table_ptr < 0x400000)
			{
				// limit of 4MB based on maximum Genesis ROM size
				iwad_ptr = 4;
				file_size -= 4;
			}
		}
	}
	else
	{
		// "IWAD" or "PWAD" not found
		printf("ERROR: WAD data not found.\n");
		system("PAUSE");
		return;
	}

	byte *data = (byte *)malloc(table_ptr - 0xC);
	fread(data, 1, table_ptr - 0xC, in_file);

	char prevEntry[9];
	prevEntry[0] = '\0';
	strcpy(prevEntry, "unknown");

	WADEntry *sprData = NULL;
	WADEntry *sprHeader = NULL;

	char entryName[9];
	entryName[8] = '\0';
	for (int i = 0; i < lump_count; i++)
	{
		unsigned int ptr;
		fread(&ptr, 4, 1, in_file);		// ptr
		ptr = swap_endian(ptr);

		unsigned int size;
		fread(&size, 4, 1, in_file);		// size
		size = swap_endian(size);

		fread(entryName, 1, 8, in_file);		// name

		WADEntry *entry = new WADEntry();
		entry->SetIsCompressed(SetEntryName(entryName, entryName));
		entry->SetName(entryName);
		//printf("lump %d/%d:  %s\n", i, lump_count, entry->GetName());

		if (entry->IsCompressed())
		{
			if (strstr(".", entry->GetName()))
			{
				char exportFile[16];
				sprintf(exportFile, "%s_c.lmp", prevEntry);
				byte *input = &data[ptr - 0xC];
				entry->SetData(decompress(input, size), size);
				DumpData(exportFile, (const byte *)entry->GetData(), entry->GetDataLength());

				sprData = entry;

				if (sprData && sprHeader)
				{
					int outputLen;
				}
				continue;
			}
			// Decompress it here, on the fly
			entry->SetIsCompressed(false);

			byte* input = &data[ptr - 0xC];

			strcpy(prevEntry, entry->GetName());

			char exportFile[16];
			sprintf(exportFile, "%s.lmp", entry->GetName());

			entry->SetData(decompress(input, size), size);
			DumpData(exportFile, (const byte*)entry->GetData(), entry->GetDataLength());
			sprHeader = entry;

			/*
			int getidbyte = 0;
			int len;
			int pos;
			int i;
			unsigned char* source;
			int idbyte = 0;

			while (1)
			{

				// get a new idbyte if necessary
				if (!getidbyte) idbyte = *input++;
				getidbyte = (getidbyte + 1) & 7;

				if (idbyte & 1)
				{
					// decompress
					pos = *input++ << LENSHIFT;
					pos = pos | (*input >> LENSHIFT);
					source = output - pos - 1;
					len = (*input++ & 0xf) + 1;
					if (len == 1) break;
					for (i = 0; i < len; i++)
						*output++ = *source++;
				}
				else {
					*output++ = *input++;
				}

				idbyte = idbyte >> 1;

			}

			entry->SetData(output, size);*/
		}
		else
		{
			char exportFile[16];
			sprintf(exportFile, "%s.lmp", entry->GetName());

			byte *input = &data[ptr - 0xC];
			entry->SetData(input, size);
			DumpData(exportFile, (byte *)entry->GetData(), entry->GetDataLength());

			sprHeader = entry;
		}

		Listable::Add(entry, (Listable **)&wadEntries);
	}

	fclose(in_file);
	free(data);
}

void ReadPC()
{
	fseek(in_file, 0, SEEK_END);
	file_size = ftell(in_file);
	rewind(in_file);

	char label[5];
	label[4] = '\0';

	// Read the file signature to verify the supplied file is an IWAD or PWAD.
	fread(label, 4, 1, in_file);
	if (!strcmp(label, "IWAD") || !strcmp(label, "PWAD"))
	{
		// "DAWI" or "DAWP" found
		fread(&lump_count, 4, 1, in_file);
		fread(&table_ptr, 4, 1, in_file);
		if (table_ptr < 0xC || table_ptr > file_size)
		{
			printf("ERROR: WAD data not found.\n");
			system("PAUSE");
			return;
		}
	}
	else
	{
		// "IWAD" or "PWAD" not found
		printf("ERROR: WAD data not found.\n");
		system("PAUSE");
		return;
	}

	byte *data = (byte *)malloc(table_ptr - 0xC);
	fread(data, 1, table_ptr - 0xC, in_file);

	char entryName[9];
	entryName[8] = '\0';
	for (int i = 0; i < lump_count; i++)
	{
		unsigned int ptr;
		fread(&ptr, 4, 1, in_file);		// ptr

		unsigned int size;
		fread(&size, 4, 1, in_file);		// size

		fread(entryName, 1, 8, in_file);		// name

		WADEntry *entry = new WADEntry();
		entry->SetIsCompressed(SetEntryName(entryName, entryName));
		entry->SetName(entryName);
		entry->SetData(&data[ptr - 0xC], size);
		Listable::Add(entry, (Listable **)&wadEntries);
	}

	fclose(in_file);
	free(data);
}

void PC2Mars()
{
	int lump;

	conversion_task = PC_2_MARS;

	fseek(in_file, 0, SEEK_END);
	file_size = ftell(in_file);
	rewind(in_file);

	char label[5];
	label[4] = '\0';

	// Read the file signature to verify the supplied file is an IWAD or PWAD.
	fread(label, 4, 1, in_file);
	if (!strcmp(label, "IWAD") || !strcmp(label, "PWAD"))
	{
		// "DAWI" or "DAWP" found
		fread(&lump_count, 4, 1, in_file);
		fread(&table_ptr, 4, 1, in_file);
		if (table_ptr < 0xC || table_ptr > file_size)
		{
			printf("ERROR: WAD data not found.\n");
			system("PAUSE");
			return;
		}
	}
	else
	{
		// "IWAD" or "PWAD" not found
		printf("ERROR: WAD data not found.\n");
		system("PAUSE");
		return;
	}

	out_table = (byte *)malloc(1);

	lump_count += 3; // fix the patch count

	data = (byte *)malloc(table_ptr - 0xC);
	fread(data, 1, table_ptr - 0xC, in_file);
	table = (byte *)malloc(lump_count * 16);
	for (int i = 0; i < lump_count; i++)
	{
		fread(&table[(i*16)], 4, 1, in_file);		// ptr
		*(int *)&table[(i*16)] = swap_endian(*(int *)&table[(i*16)]);

		fread(&table[(i*16)+4], 4, 1, in_file);		// size
		*(int *)&table[(i*16)+4] = swap_endian(*(int *)&table[(i*16)+4]);

		fread(&table[(i*16)+8], 1, 8, in_file);		// name
	}

	for (int i = 0; i < lump_count; i++)
	{
		*(int *)&table[i*16] = swap_endian(*(int *)&table[i*16]);
		*(int *)&table[(i*16)+4] = swap_endian(*(int *)&table[(i*16)+4]);

		// Set PLAYPAL to PLAYPALS
		// TODO: Jaguar does not have a 'PLAYPALS'
		if (!memcmp(&table[(i * 16) + 8], "PLAYPAL", strlen("PLAYPAL")))
			table[(i * 16) + 15] = 'S';
	}
	fclose(in_file);

#ifdef IMPORT_MODE_WAD
	strcpy(label, "DAWI"); // make sure we're creating an IWAD
	fwrite(&label, 4, 1, out_file);
	fseek(out_file, 8, SEEK_CUR);
	iwad_ptr = 0;
#else
	fseek(out_file, 0, SEEK_END);
	out_file_size = ftell(out_file);
	rewind(out_file);

	// Check for "DAWI" marker at start of file
	fread(&label, 4, 1, out_file);
	if (!strcmp(label, "DAWI"))
	{
		// "DAWI" found
		fread(&out_lump_count, 4, 1, out_file);
		out_lump_count = swap_endian(out_lump_count);
		if (out_lump_count > 0 && out_lump_count <= 2048)
		{
			// limit based on "MAXLUMPS" in Jaguar source
			fread(&out_table_ptr, 4, 1, out_file);
			out_table_ptr = swap_endian(out_table_ptr);
			if (out_table_ptr >= 0xC && out_table_ptr < 0x400000)
			{
				// limit of 4MB based on maximum Genesis ROM size
				iwad_ptr = 4; // The position right after DAWI in the file
				fseek(out_file, 4 + 0xC, SEEK_SET);	// required because this is using "r+b"
				out_file_size = 0;
			}
		}
	}
	else
	{
		// "DAWI" not found"
		printf("ERROR: WAD data not found.\n");
		system("PAUSE");
		return;
	}
#endif

	out_file_size = 0xC;
	out_lump_count = lump_count;

	for (lump = 0; lump < lump_count-3; lump++)
	{
		char entryName[9];

		WADEntry *entry = new WADEntry();
		entry->SetIsCompressed(SetEntryName(entryName, (const char *)&table[(lump * 16) + 8]));
		entry->SetName(entryName);
		Listable::Add(entry, (Listable **)&wadEntries);
		// TODO: Add data via entry->SetData()

		printf("0x%04X\t%s\n", lump + lump_value_fix, entryName);
		switch(type)
		{
			case TYPE_TEXTURE:
				if (!strcmp(entryName, "P_END"))
				{
					WriteTableEnd(TYPE_TEXTURE);
					type = 0;
					if (floor_end_written)
					{
						lump_value_fix++;
						printf("0x%04X\tTEXTURE1\n", lump + lump_value_fix);
						CreateTEXTURE1(lump);
					}
				}
				else if (IsInExclusionList(entryName))
				{
					lump_value_fix--;
				}
				else
				{
					ConvertTextureData32X(lump);
				}
				break;
			case TYPE_FLOOR:
				if (!strcmp(entryName, "F_END"))
				{
					WriteTableEnd(TYPE_FLOOR);
					type = 0;
					if (texture_end_written)
					{
						lump_value_fix++;
						printf("0x%04X\tTEXTURE1\n", lump + lump_value_fix);
						CreateTEXTURE1(lump);
					}
				}
				else if (IsInExclusionList(entryName))
				{
					lump_value_fix--;
				}
				else
					ConvertFloorData(lump);
				break;
			case TYPE_SPRITE:
				if (!strcmp(entryName, "S_END"))
				{
					//WriteTableEnd(TYPE_SPRITE);
					lump_value_fix--;
					type = 0;
				}
				else
				{
					ConvertSpriteDataFromPCToJag(lump);
					lump_value_fix++;
					out_lump_count++;
				}
				break;
			default:
				if (!strcmp(entryName, "P_START"))
				{
					WriteTableStart(TYPE_TEXTURE);
					type = TYPE_TEXTURE;
				}
				else if (!strcmp(entryName, "F_START"))
				{
					WriteTableStart(TYPE_FLOOR);
					type = TYPE_FLOOR;
				}
				else if (!strcmp(entryName, "S_START"))
				{
					//WriteTableStart(TYPE_SPRITE);
					lump_value_fix--;
					type = TYPE_SPRITE;
				}
				else if (!strcmp(entryName, "_32X_"))
				{
					Read_32X_(lump);
					lump_value_fix--;
				}
				else if (!strcmp(entryName, "COLORMAP"))
				{
					ConvertCOLORMAPData32X(lump);
				}
				else if (!strcmp(entryName, "PLAYPALS"))
				{
					ConvertPLAYPALData(lump);	// used for both PC and 32X conversions
				}
				else if (IsInExclusionList(entryName))
				{
					lump_value_fix--;
				}
				else if (!strcmp(entryName, "THINGS"))
				{
					ConvertMapData32X(lump, THINGS);
				}
				else if (!strcmp(entryName, "LINEDEFS"))
				{
					ConvertMapData32X(lump, LINEDEFS);
				}
				else if (!strcmp(entryName, "SIDEDEFS"))
				{
					ConvertMapData32X(lump, SIDEDEFS);
				}
				else if (!strcmp(entryName, "VERTEXES"))
				{
					ConvertMapData32X(lump, VERTEXES);
				}
				else if (!strcmp(entryName, "SEGS"))
				{
					ConvertMapData32X(lump, SEGS);
				}
				else if (!strcmp(entryName, "SSECTORS"))
				{
					ConvertMapData32X(lump, SSECTORS);
				}
				else if (!strcmp(entryName, "NODES"))
				{
					ConvertMapData32X(lump, NODES);
				}
				else if (!strcmp(entryName, "SECTORS"))
				{
					ConvertMapData32X(lump, SECTORS);
				}
				else if (!strcmp(entryName, "REJECT"))
				{
					ConvertMapData32X(lump, REJECT);
				}
				else if (!strcmp(entryName, "BLOCKMAP"))
				{
					ConvertMapData32X(lump, BLOCKMAP);
				}
				else
					ConvertRawData(lump);
				break;
		}
	}

	out_lump_count -= 7;
#ifndef FORCE_LITTLE_ENDIAN
	int i = swap_endian(ftell(out_file) - iwad_ptr);
	out_lump_count = swap_endian(out_lump_count);
#else
	i = ftell(out_file) - iwad_ptr;
#endif

	fwrite(out_table, 1, out_table_size, out_file);

#ifndef IMPORT_MODE_WAD
	printf("\n");

	// Adjust ROM size
	int n = ftell(out_file);
	printf("ROM size = %i.%iMB\n", n>>20, (int)(((float)((n >> 16) & 0xF) / 16.0f) * 10));
	while ((n & 0xFFFFF) != 0)
	{
		fputc(0xFF, out_file);
		n++;
	}
	fseek(out_file, 0x1A4, SEEK_SET);
	n--;
	n = swap_endian(n);
	fwrite(&n, 4, 1, out_file);
	n = swap_endian(n);
	n++;
	printf("File size = %i.%iMB\n", n>>20, (int)(((float)((n >> 16) & 0xF) / 16.0f) * 10));
	if (n > 0x400000)
		printf("WARNING: ROM exceeds the Sega Genesis ROM size limitation of 4MB.\n");
#endif

	fseek(out_file, iwad_ptr + 4, SEEK_SET);
	fwrite(&out_lump_count, 4, 1, out_file);
	fwrite(&i, 4, 1, out_file);

#ifndef IMPORT_MODE_WAD
	// Fix checksum
	checksum = 0;
	fseek(out_file, 0x200, SEEK_SET);
	for (t = 0x200; t < n; t += 2)
	{
		fread(&d, 2, 1, out_file);
		d = (swap_endian(d) >> 16);
		checksum += d;
	}
	fseek(out_file, 0x18E, SEEK_SET);
	checksum = (swap_endian(checksum) >> 16);
	fwrite(&checksum, 2, 1, out_file);
	checksum = (swap_endian(checksum) >> 16);
	printf("Checksum = 0x%04X\n", checksum);
#endif

	printf("\nImport finished!\n");
	system("PAUSE");
	return;
}

void Mars2PC()
{
}
/*
void Mars2PC()
{
	// Conversion: [32X] ---->> [PC]

	int lump;

	conversion_task = MARS_2_PC;

	fseek(in_file, 0, SEEK_END);
	file_size = ftell(in_file);
	rewind(in_file);

	for (int i = 0; i < file_size; i += 4)
	{
		// Search the ROM for the "IWAD" signature to locate the WAD data.
		fread(&label, 4, 1, in_file);
		//if (label == 0x44415749 || label == 0x44415750)  // TODO: Double-check. Pretty sure this was a mistake. Shouldn't look for "PWAD".
		if (label == 0x44415749)
		{
			// "DAWI" found
			fread(&lump_count, 4, 1, in_file);
			lump_count = swap_endian(lump_count);
			if(lump_count > 0 && lump_count <= 2048)
			{
				// limit based on "MAXLUMPS" in Jaguar source
				fread(&table_ptr, 4, 1, in_file);
				table_ptr = swap_endian(table_ptr);
				if(table_ptr >= 0xC && table_ptr < 0x400000)
				{
					// limit of 4MB based on maximum Genesis ROM size
					iwad_ptr = i;
					file_size -= i;
					break;
				}
			}
		}
	}

	//if (label != 0x44415749 && label != 0x44415750)	// TODO: Double-check. Pretty sure this was a mistake. Shouldn't care about "PWAD".
	if (label != 0x44415749)
	{
		// "DAWI" or "DAWP" not found"
		printf("ERROR: WAD data not found.\n");
		system("PAUSE");
		return;
	}

	out_table = (byte *)malloc(1);

	lump_count += 2;	// fix the patch count
	lump_count++;	// make room for PNAMES

	data = (byte *)malloc(table_ptr - 0xC);
	fread(data, 1, table_ptr - 0xC, in_file);
	table = (byte *)malloc(lump_count * 16);
	fread(table, 1, lump_count * 16, in_file);
	for (i = 0; i < lump_count; i++)
	{
		unsigned int firstWord = *(int *)&table[i * 16];
		unsigned int secondWord = *(int *)&table[(i * 16) + 4];
		*(int *)&table[i*16] = swap_endian(firstWord);
		*(int *)&table[(i*16)+4] = swap_endian(secondWord);

		char entryName[9];
		bool isCompressed = SetEntryName(entryName, (const char*)&table[(i * 16) + 8]);

		if (isCompressed)
			printf("%s is compressed.\n", entryName);

		// this part is needed to get size information about textures
		if (!strcmp(entryName, "TEXTURE1"))
		{
			texture1_ptr = *(int *)&table[i * 16];
			texture1_size = *(int *)&table[(i * 16) + 4];

			fseek(in_file, iwad_ptr + texture1_ptr, SEEK_SET);
			texture1 = (byte *)malloc(texture1_size);
			fread(texture1, 1, texture1_size, in_file);
		}
		else if (!strcmp(entryName, "PLAYPALS"))
		{
			table[(i * 16) + 15] = 0;
		}
	}
	fclose(in_file);

	label = 0x44415750;	// make sure we're creating an PWAD
	fwrite(&label, 4, 1, out_file);
	fseek(out_file, 8, SEEK_CUR);

	out_lump_count = lump_count;

	// Put 32X label in WAD file
	Create_32X_();
	lump_value_fix++;
	out_lump_count++;

	for (lump=0; lump < lump_count-3; lump++)
	{
		printf("0x%04X\t%c%c%c%c%c%c%c%c\n", lump + lump_value_fix,
		 (table[(lump*16)+8] & 0x7F), table[(lump*16)+9], table[(lump*16)+10], table[(lump*16)+11],
		 table[(lump*16)+12], table[(lump*16)+13], table[(lump*16)+14], table[(lump*16)+15]);

		char entryName[9];
		SetEntryName(entryName, (const char*)&table[(lump * 16) + 8]);

		switch(type)
		{
			case TYPE_TEXTURE:
				if (strcmp(entryName, "T_END") == 0)
				{
					WriteTableEnd(TYPE_TEXTURE);
					type = 0;
				}
				else
				{
					ConvertTextureData(lump);
				}
				break;
			case TYPE_FLOOR:
				if (strcmp(entryName, "F_END") == 0)
				{
					WriteTableEnd(TYPE_FLOOR);
					type = 0;
				}
				else
					ConvertFloorData(lump);
				break;
			default:
				if (lump < lump_count-1)
				{
					if (table[(lump*16)+8+16] == '.' && table[(lump*16)+9+16] == 0)
					{
						if (first_sprite)
						{
							if(!first_texture)
							{
								WriteTableEnd(TYPE_TEXTURE);
								first_texture = 1;
							}
							else if(!first_floor)
							{
								WriteTableEnd(TYPE_FLOOR);
								first_floor = 1;
							}

							first_sprite = 0;
							WriteTableStart(TYPE_SPRITE);
						}
						ConvertSpriteData(lump);
						lump++;
						out_lump_count--;
					}
					else if (strcmp((char*)&table[(lump*16)+9], "_START") == 0){
						if (table[(lump*16)+8] == 'T')
						{
							if(!first_floor)
							{
								WriteTableEnd(TYPE_FLOOR);
								first_floor = 1;
							}
							else if(!first_sprite)
							{
								WriteTableEnd(TYPE_SPRITE);
								first_sprite = 1;
							}

							WriteTableStart(TYPE_TEXTURE);
							type = TYPE_TEXTURE;
						}
						else if (table[(lump*16)+8] == 'F')
						{
							if (!first_floor)
							{
								WriteTableEnd(TYPE_TEXTURE);
								first_texture = 1;
							}
							else if (!first_sprite)
							{
								WriteTableEnd(TYPE_SPRITE);
								first_sprite = 1;
							}

							WriteTableStart(TYPE_FLOOR);
							type = TYPE_FLOOR;
						}
					}
					else if (strcmp((const char*)&table[(lump*16)+9], "_END") == 0){
						if (table[(lump*16)+8] == 'T')
							WriteTableEnd(TYPE_TEXTURE);
						else if (table[(lump*16)+8] == 'F')
							WriteTableEnd(TYPE_FLOOR);
						type = 0;
					}
					else if (!strcmp(entryName, "COLORMAP"))
					{
						ConvertCOLORMAPData(lump);
					}
					else if (!strcmp(entryName, "PLAYPAL"))
					{
						ConvertPLAYPALData(lump);
					}
					else if (!strcmp(entryName, "TEXTURE1"))
					{
						ConvertTEXTURE1Data(lump);
						lump_value_fix++;
						printf("0x%04X\tPNAMES\n", lump + lump_value_fix);
						CreatePNAMES(lump);
					}
					else if (!strcmp(entryName, "THINGS"))
					{
						ConvertMapData(lump, THINGS);
					}
					else if (!strcmp(entryName, "LINEDEFS"))
					{
						ConvertMapData(lump, LINEDEFS);
					}
					else if (!strcmp(entryName, "SIDEDEFS"))
					{
						ConvertMapData(lump, SIDEDEFS);
					}
					else if (!strcmp(entryName, "VERTEXES"))
					{
						ConvertMapData(lump, VERTEXES);
					}
					else if (!strcmp(entryName, "SEGS"))
					{
						ConvertMapData(lump, SEGS);
					}
					else if (!strcmp(entryName, "SSECTORS"))
					{
						ConvertMapData(lump, SSECTORS);
					}
					else if (!strcmp(entryName, "NODES"))
					{
						ConvertMapData(lump, NODES);
					}
					else if (!strcmp(entryName, "SECTORS"))
					{
						ConvertMapData(lump, SECTORS);
					}
					else if (!strcmp(entryName, "REJECT"))
					{
						ConvertMapData(lump, REJECT);
					}
					else if (!strcmp(entryName, "BLOCKMAP"))
					{
						ConvertMapData(lump, BLOCKMAP);
					}
					else
					{
						if(!first_texture)
						{
							WriteTableEnd(TYPE_TEXTURE);
							first_texture = 1;
						}
						else if(!first_floor)
						{
							WriteTableEnd(TYPE_FLOOR);
							first_floor = 1;
						}
						else if(!first_sprite)
						{
							WriteTableEnd(TYPE_SPRITE);
							first_sprite = 1;
						}
						ConvertRawData(lump);
					}
				}
				else if (!strcmp(entryName, "TEXTURE1"))
				{
					ConvertRawData(lump);
					lump_value_fix++;
					printf("0x%04X\tPNAMES\n", lump + lump_value_fix);
					CreatePNAMES(lump);
				}
				else if (!strcmp(entryName, "THINGS"))
				{
					ConvertMapData(lump, THINGS);
				}
				else if (!strcmp(entryName, "LINEDEFS"))
				{
					ConvertMapData(lump, LINEDEFS);
				}
				else if (!strcmp(entryName, "SIDEDEFS"))
				{
					ConvertMapData(lump, SIDEDEFS);
				}
				else if (!strcmp(entryName, "VERTEXES"))
				{
					ConvertMapData(lump, VERTEXES);
				}
				else if (!strcmp(entryName, "SEGS"))
				{
					ConvertMapData(lump, SEGS);
				}
				else if (!strcmp(entryName, "SSECTORS"))
				{
					ConvertMapData(lump, SSECTORS);
				}
				else if (!strcmp(entryName, "NODES"))
				{
					ConvertMapData(lump, NODES);
				}
				else if (!strcmp(entryName, "SECTORS"))
				{
					ConvertMapData(lump, SECTORS);
				}
				else if (!strcmp(entryName, "REJECT"))
				{
					ConvertMapData(lump, REJECT);
				}
				else if (!strcmp(entryName, "BLOCKMAP"))
				{
					ConvertMapData(lump, BLOCKMAP);
				}
				else
				{
					ConvertRawData(lump);
				}
				break;
		}
	}

	i = ftell(out_file);
	fwrite(out_table, 1, out_table_size, out_file);

	fseek(out_file, 4, SEEK_SET);
	fwrite(&out_lump_count, 4, 1, out_file);
	fwrite(&i, 4, 1, out_file);

	printf("\nExport finished!\n");
	system("PAUSE");
	return;
}*/

// Read the "_32X_" lump to determine conversion behavior. Used to support WAD
// files converted with previous versions of the software.
void Read_32X_(int lump)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	if (data_size < 9)
	{
		format = 0;
	}
	else
	{
		if (!memcmp(&data[data_ptr], "WAD32X", strlen("WAD32X")))
		{
			format = data[data_ptr + 8];
			if(format > WAD_FORMAT)
				printf("WARNING: WAD format %i not supported. Assuming format %i.\n", format, WAD_FORMAT);
		}
		else
			format = 0;
	}
}

// Create the "_32X_" lump to give the software an understanding of what needs
// to happen when importing it back into the game. Used to ensure
// compatibility with future versions of the software.
void Create_32X_()
{
	const char *string = "WAD32X";
	const short ver = (short)(VERSION * 100);
	const short format = WAD_FORMAT;

	fwrite(string, 1, strlen(string), out_file);
	fwrite(&ver, 2, 1, out_file);
	fwrite(&format, 2, 1, out_file);

	WriteTableCustom(out_file_size, 10, "_32X_");

	out_file_size += 10;
}

void CreatePNAMES(int lump)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];
	char name[9];

	int i;

	name[8] = '\0';

	for (i = 0x12C; i < data_size; i += 0x20)
	{
		memcpy(name, &data[data_ptr + i], 8);
		WritePatchName(name);
	}

	fwrite(pnames_table, 1, 4+(patches_count<<3), out_file);

	WriteTableCustom(out_file_size, 4+(patches_count<<3), "PNAMES");

	out_file_size += (4+(patches_count<<3));
}

void CreateTEXTURE1(int lump)
{
	// Before writing the data, we sort the textures in ABC order purely for
	// convention purposes.
	unsigned long long *texture1_names = NULL;
	unsigned short *texture1_order = NULL;

	if (num_of_textures > 0)
	{
		texture1_names = (unsigned long long *)malloc(num_of_textures<<3);
		texture1_order = (unsigned short *)calloc(num_of_textures, 2);
	}

	for (int i = 0; i < num_of_textures; i++)
	{
		//texture1_names[i] = *(long long *)&out_texture1_data[i<<5];

		texture1_names[i] = out_texture1_data[(i<<5)+0];
		texture1_names[i] <<= 8;
		texture1_names[i] |= out_texture1_data[(i<<5)+1];
		texture1_names[i] <<= 8;
		texture1_names[i] |= out_texture1_data[(i<<5)+2];
		texture1_names[i] <<= 8;
		texture1_names[i] |= out_texture1_data[(i<<5)+3];
		texture1_names[i] <<= 8;
		texture1_names[i] |= out_texture1_data[(i<<5)+4];
		texture1_names[i] <<= 8;
		texture1_names[i] |= out_texture1_data[(i<<5)+5];
		texture1_names[i] <<= 8;
		texture1_names[i] |= out_texture1_data[(i<<5)+6];
		texture1_names[i] <<= 8;
		texture1_names[i] |= out_texture1_data[(i<<5)+7];
	}

	for (int i = 0; i < num_of_textures; i++)
	{
		for (int n = 0; n < num_of_textures; n++)
		{
			if(texture1_names[i] > texture1_names[n])
				texture1_order[i]++;
		}
	}

	fwrite(out_texture1_table, 1, (num_of_textures<<2) + 4, out_file);

	for (int i = 0; i < num_of_textures; i++)
	{
		int n = 0;
		while (texture1_order[n] != i)
			n++;

		*(short *)&out_texture1_data[(n << 5) + 26] = i;
		fwrite(&out_texture1_data[n << 5], 1, 32, out_file);
	}

	WriteTableCustom(out_file_size, (num_of_textures<<2) + (num_of_textures<<5) + 4, "TEXTURE1");

	out_file_size += (num_of_textures<<2) + (num_of_textures<<5) + 4;

	if(num_of_textures > 0)
	{
		free(texture1_names);
		free(texture1_order);
	}
}

void ConvertTEXTURE1Data(int lump)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	int i;

	int num_of_textures = *(int *)&data[data_ptr];

	int *texture_ptr_table;
	char *texture_data_table;

	// Our task with this function is to copy TEXTURE1 with "AASHITTY" inserted
	texture_ptr_table = (int *)malloc((num_of_textures+1) << 2);
	texture_data_table = (char *)malloc((num_of_textures+1) << 5);

	// Copy PTR table
	texture_ptr_table[0] = ((num_of_textures+1)<<2) + 4;
	for (i = 1; i < num_of_textures+1; i++)
		texture_ptr_table[i] = *(int *)&data[data_ptr + 4 - 4 + (i<<2)] + 32 + 4;

	// Copy DATA table
	texture_data_table[0] = 'A';
	texture_data_table[1] = 'A';
	texture_data_table[2] = 'S';
	texture_data_table[3] = 'H';
	texture_data_table[4] = 'I';
	texture_data_table[5] = 'T';
	texture_data_table[6] = 'T';
	texture_data_table[7] = 'Y';
	*(int *)&texture_data_table[8] = 0;
	*(short *)&texture_data_table[12] = 64;
	*(short *)&texture_data_table[14] = 128;
	*(int *)&texture_data_table[16] = 0;
	*(short *)&texture_data_table[20] = 1;
	*(short *)&texture_data_table[22] = 0;
	*(short *)&texture_data_table[24] = 0;
	*(short *)&texture_data_table[26] = 0;
	*(short *)&texture_data_table[28] = 1;
	*(short *)&texture_data_table[30] = 0;

	for(i=0; i < (num_of_textures<<5); i++)
		texture_data_table[32 + i] = data[data_ptr + 4 + (num_of_textures<<2) + i];

	num_of_textures++;
	fwrite(&num_of_textures, 4, 1, out_file);
	num_of_textures--;

	fwrite(texture_ptr_table, 4, num_of_textures+1, out_file);

	fwrite(texture_data_table, 1, (num_of_textures+1)<<5, out_file);

	free(texture_ptr_table);
	free(texture_data_table);

	WriteTable(lump, out_file_size, 4 + ((num_of_textures+1)<<2) + ((num_of_textures+1)<<5));

	out_file_size += (4 + ((num_of_textures+1)<<2) + ((num_of_textures+1)<<5));
}

void ConvertPLAYPALData(int lump)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	unsigned short color;

	for (color = 0; color < 11520; color++)
		fputc(data[data_ptr + (color % data_size)], out_file);

	WriteTable(lump, out_file_size, 11520);

	out_file_size += 11520;
}

void ConvertCOLORMAPData32X(int lump)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	unsigned short color;
	unsigned short shade;

	if (data_size == 8704)
	{
		// COLORMAP doesn't contain the extra 32X data
		for (shade = 0; shade < 32*256; shade += 256)
		{
			for (color = 128; color < 256+128; color++)
			{
				fputc(data[data_ptr + (color % 256) + shade], out_file);
				fputc(data[data_ptr + (color % 256) + shade], out_file);
			}
		}

		WriteTable(lump, out_file_size, 16384);

		out_file_size += 16384;
	}
	else if (format == 0)
	{
		// this is for backwards compatibility with v0.99 and v1.00 WAD files
		fwrite(&data[data_ptr], 1, data_size, out_file);

		WriteTable(lump, out_file_size, data_size);

		out_file_size += data_size;
	}
	else
	{
		for (shade = 0; shade < 32*256; shade += 256)
		{
			for (color = 128; color < 256+128; color++)
			{
				fputc(data[data_ptr + (color % 256) + shade], out_file);
				fputc(data[data_ptr + (color % 256) + shade + 8704], out_file);
			}
		}

		WriteTable(lump, out_file_size, 16384);

		out_file_size += 16384;
	}
}

void ConvertCOLORMAPData(int lump)
{
	char inv_colormap[0x100] =
	{
		0x04, 0x51, 0x50, 0x59, 0x00, 0x51, 0x50, 0x50, 0x04, 0x55, 0x53, 0x51, 0x50, 0x57, 0x56, 0x54,
		0x6D, 0x6C, 0x6A, 0x03, 0x68, 0x66, 0x65, 0x64, 0x62, 0x61, 0x60, 0x5F, 0x5E, 0x5D, 0x5C, 0x5B,
		0x5A, 0x59, 0x58, 0x57, 0x57, 0x56, 0x55, 0x55, 0x54, 0x53, 0x52, 0x52, 0x52, 0x51, 0x51, 0x50,
		0x07, 0x06, 0x06, 0x05, 0x6F, 0x6E, 0x6D, 0x6D, 0x6C, 0x6A, 0x69, 0x68, 0x67, 0x65, 0x64, 0x64,
		0x62, 0x61, 0x60, 0x60, 0x5F, 0x5E, 0x5C, 0x5C, 0x5B, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53,
		0x07, 0x06, 0x05, 0x05, 0x6E, 0x6D, 0x6D, 0x6B, 0x6A, 0x69, 0x68, 0x67, 0x66, 0x65, 0x64, 0x63,
		0x62, 0x61, 0x60, 0x5F, 0x5E, 0x5D, 0x5C, 0x5B, 0x5A, 0x58, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53,
		0x6C, 0x6A, 0x68, 0x66, 0x64, 0x62, 0x60, 0x5F, 0x5D, 0x5B, 0x59, 0x57, 0x55, 0x53, 0x51, 0x50,
		0x68, 0x67, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5F, 0x5D, 0x5D, 0x5C, 0x5B, 0x5A, 0x58, 0x58,
		0x62, 0x60, 0x5F, 0x5D, 0x5B, 0x59, 0x57, 0x55, 0x60, 0x5F, 0x5D, 0x5C, 0x5A, 0x59, 0x57, 0x56,
		0x07, 0x6D, 0x69, 0x65, 0x61, 0x5D, 0x5A, 0x56, 0x00, 0x06, 0x6E, 0x6A, 0x66, 0x63, 0x60, 0x5C,
		0x59, 0x58, 0x57, 0x57, 0x57, 0x56, 0x55, 0x55, 0x54, 0x54, 0x53, 0x53, 0x52, 0x51, 0x51, 0x50,
		0x07, 0x6E, 0x6A, 0x66, 0x62, 0x5E, 0x5A, 0x57, 0x53, 0x52, 0x52, 0x51, 0x51, 0x50, 0x50, 0x50,
		0x00, 0x07, 0x05, 0x6E, 0x6C, 0x69, 0x67, 0x65, 0x63, 0x62, 0x61, 0x60, 0x5F, 0x5D, 0x5C, 0x5B,
		0x00, 0x00, 0x08, 0x07, 0x07, 0x06, 0x05, 0x05, 0x5B, 0x59, 0x58, 0x57, 0x57, 0x55, 0x53, 0x52,
		0x50, 0x50, 0x50, 0x04, 0x04, 0x04, 0x04, 0x04, 0x03, 0x6F, 0x69, 0x5F, 0x5B, 0x58, 0x55, 0x61
	};

	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	//int data_size = *(int *)&table[(lump*16)+4];

	unsigned short color;
	unsigned short shade;

	// regular colors
	for (shade = 0; shade < 32*256*2; shade += (256*2))
	{
		for (color = (128*2); color < (256+128)*2; color += 2)
			fputc(data[data_ptr + (color % (256*2)) + shade], out_file);
	}

	// invulnerability colors
	for (color = 0; color < 256; color++)
		fputc(inv_colormap[color], out_file);
	for (color = 0; color < 256; color++)
		fputc(0, out_file);

	// interlacing colors (for when data is ported back to the 32X)
	for (shade = 0; shade < 32*256*2; shade += (256*2))
	{
		for (color = (128*2)+1; color < (256+128)*2; color += 2)
			fputc(data[data_ptr + (color % (256*2)) + shade], out_file);
	}

	WriteTable(lump, out_file_size, 16896);

	out_file_size += 16896;
}

void ConvertMapData32X(int lump, char mapfile)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	byte *output = 0;
	int output_size = 0;

	byte *uncompressed = 0;
	int compressed_size = 0;

	byte *key = 0;
	int key_size = 0;

	byte *bitfield = 0;
	int bitfield_size = 0;

	int n;	//
	int i;	//
	int b;	//
	int c;	// number of bytes to compare

	int data_comp_1;
	int data_comp_2;
	char bytes_to_copy = 0;

	switch (mapfile)
	{
		case THINGS:
		case LINEDEFS:
		case SIDEDEFS:
		case SEGS:
		case SSECTORS:
		case SECTORS:
			output_size = data_size;	// must be uncompressed size

			key = (byte *)malloc(1);
			bitfield = (byte *)malloc(1);

			table[(lump*16)+8] |= 0x80;

			uncompressed = (byte *)malloc(data_size);
			for (i = 0; i < data_size; i++)
				uncompressed[i] = data[data_ptr + i];
			break;
		case VERTEXES:
			output_size = data_size<<1;
			output = (byte *)malloc(data_size<<1);
			for (i = 0; i < data_size<<1; i += 4)
			{
				output[i] = data[data_ptr + (i>>1)+1];
				output[i+1] = data[data_ptr + (i>>1)];
				*(unsigned short *)&output[i+2] = 0;
			}
			break;
		case NODES:
			output_size = data_size<<1;
			output = (byte *)malloc(data_size<<1);
			for (i = 0; i < data_size<<1; i += 4)
			{
				if ((i % 56) >= 48)
				{
					*(unsigned short *)&output[i] = 0;
					output[i+2] = data[data_ptr + (i>>1)+1];
					output[i+3] = data[data_ptr + (i>>1)];
				}
				else
				{
					output[i] = data[data_ptr + (i>>1)+1];
					output[i+1] = data[data_ptr + (i>>1)];
					*(unsigned short *)&output[i+2] = 0;
				}
			}
			break;
		case REJECT:
			output_size = data_size;
			output = (byte *)malloc(data_size);
			for (i = 0; i < data_size; i++)
				output[i] = data[data_ptr + i];
			break;
		case BLOCKMAP:
			output_size = data_size;
			output = (byte *)malloc(data_size);
			for (i = 0; i < data_size; i += 2)
			{
				output[i] = data[data_ptr + i+1];
				output[i+1] = data[data_ptr + i];
			}
			break;
	}

	if (table[(lump*16)+8] >> 7)
	{
		// data needs to be compressed
		for (i = 1; i <= output_size - 3; i++)
		{
			// check for repeated data (no less than 3 bytes in length)
			data_comp_1 = (uncompressed[i]<<16)
			 | (uncompressed[i+1]<<8)
			 | uncompressed[i+2];

			for (n = 0; n < i; n++)
			{
				if (i-n > 0x1000)
					n = i - 0x1000;

				data_comp_2 = (uncompressed[n]<<16)
				 | (uncompressed[n + (1%(i-n))]<<8)
				 | uncompressed[n + (2%(i-n))];

				if (data_comp_1 == data_comp_2)
				{
					// check to see how many bytes match beyond 3

					if (bytes_to_copy == 0)
					{
						bytes_to_copy = 3;
						b = n;
					}
					for (c=3; c <= 16; c++)
					{
						if (i+c <= output_size)
						{
							if (uncompressed[i+c] != uncompressed[n + (c%(i-n))])
							{
								// stop checking for matches
								if(c >= bytes_to_copy)
								{
									bytes_to_copy = c;
									c = 0x40;
									b = n;		// save 'n' for referencing later
								}
								else
									c = 0x40;
							}
						}
						else
						{
							if (c >= bytes_to_copy)
							{
								bytes_to_copy = c-1;
								c = 0x40;
								b = n;		// save 'n' for referencing later
							}
							else
								c = 0x40;
						}
					}

					if (c == 17)
					{
						bytes_to_copy = 16;
						b = n;
					}
				}
			}

			if (bytes_to_copy)
			{
				// add info to the key

				key_size += 4;
				key = (byte *)realloc(key, key_size);

				*(unsigned short *)&key[key_size - 4] = i;
				*(unsigned short *)&key[key_size - 2] = (((i-b)-1)<<4) | (bytes_to_copy-1);

				i += (bytes_to_copy-1);

				bytes_to_copy = 0;
			}
		}

		key_size += 4;
		key = (byte *)realloc(key, key_size);

		*(unsigned short *)&key[key_size - 4] = 0xFFFF;
		*(unsigned short *)&key[key_size - 2] = 0;

		// create key bitfield
		n = 0;
		for (i = 0; i < output_size; i++)
		{
			if(i < *(unsigned short *)&key[n])
			{
				bitfield_size++;
				bitfield = (byte *)realloc(bitfield, bitfield_size);

				bitfield[bitfield_size-1] = 0;
			}
			else
			{
				bitfield_size++;
				bitfield = (byte *)realloc(bitfield, bitfield_size);

				bitfield[bitfield_size-1] = 1;

				i += (*(unsigned short *)&key[n+2] & 0xF);
				n += 4;
			}
		}

		// add terminator to key
		bitfield_size++;
		bitfield = (byte *)realloc(bitfield, bitfield_size);
		bitfield[bitfield_size-1] = 0xF;

		// if needed, align the bitfield to the next whole byte (8 bits)
		while (bitfield_size & 7)
		{
			bitfield_size++;
			bitfield = (byte *)realloc(bitfield, bitfield_size);

			bitfield[bitfield_size-1] = 0;
		}

		// write data
		n = 0;	// used as subscript to key[]
		b = 0;	// used as subscript to bitfield[]
		for (i = 0; i < output_size; i++)
		{
			if (i < output_size)
			{
				if ((b & 7) == 0)
				{
					data_comp_1 = 0;

					if(bitfield[b] == 0xF)	data_comp_1 |= 1;
					else					data_comp_1 |= bitfield[b];

					if(bitfield[b+1] == 0xF)	data_comp_1 |= 2;
					else					data_comp_1 |= (bitfield[b+1]<<1);

					if(bitfield[b+2] == 0xF)	data_comp_1 |= 4;
					else					data_comp_1 |= (bitfield[b+2]<<2);

					if(bitfield[b+3] == 0xF)	data_comp_1 |= 8;
					else					data_comp_1 |= (bitfield[b+3]<<3);

					if(bitfield[b+4] == 0xF)	data_comp_1 |= 16;
					else					data_comp_1 |= (bitfield[b+4]<<4);

					if(bitfield[b+5] == 0xF)	data_comp_1 |= 32;
					else					data_comp_1 |= (bitfield[b+5]<<5);

					if(bitfield[b+6] == 0xF)	data_comp_1 |= 64;
					else					data_comp_1 |= (bitfield[b+6]<<6);

					if(bitfield[b+7] == 0xF)	data_comp_1 |= 128;
					else					data_comp_1 |= (bitfield[b+7]<<7);

					fputc(data_comp_1, out_file);
					compressed_size++;
				}

				if (data_comp_1 & 1)
				{
					fputc(key[n+3], out_file);
					fputc(key[n+2], out_file);
					compressed_size += 2;

					i += (*(unsigned short *)&key[n+2] & 0xF);
					n += 4;
				}
				else
				{
					fputc(uncompressed[i], out_file);
					compressed_size++;
				}

				data_comp_1 >>= 1;
				b++;
			}
		}

		if ((b&7) == 0)
		{
			if (bitfield[b] == 0xF)
			{
				// let 'i' go one over to allow special cases
				// where terminator needs to be inserted
				// (e.g. MAP14 "SEGS")
				b = 1;
				fputc(b, out_file);
				compressed_size++;
			}
		}

		// add terminator
		compressed_size += 2;
		i = 0;
		fwrite(&i, 2, 1, out_file);

		while (compressed_size & 3){
			fputc(i, out_file);
			compressed_size++;
		}

		WriteTable(lump, out_file_size, output_size);

		out_file_size += compressed_size;
	}
	else
	{
		fwrite(output, 1, output_size, out_file);

		i = 0;
		n = output_size;
		while (n & 3)
		{
			fputc(i, out_file);
			n++;
		}

		WriteTable(lump, out_file_size, output_size);

		out_file_size += output_size;

		n = output_size;
		while (n & 3){
			n++;
			out_file_size++;
		}
	}

	if (output)
		free(output);
	if (uncompressed)
		free(uncompressed);
	if (key)
		free(key);
	if (bitfield)
		free(bitfield);
}

void ConvertMapData(int lump, char mapfile)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	byte *uncompressed;
	byte *converted = NULL;

	int ci;
	int i;

	char key;
	char key_bit;

	int key_location;
	char key_count;

	uncompressed = (byte *)malloc(data_size + 16);	// add 16 to make sure we have enough room

	if (table[(lump*16)+8] >> 7){
		// data must be decompressed

		table[(lump*16)+8] &= 0x7F;

		ci = 0;
		i = 0;
		while (i < data_size)
		{
			key = data[data_ptr + ci];
			ci++;
			for (key_bit = 0; key_bit < 8; key_bit++)
			{
				if (key & 1){
					// copy data from location
					key_location = (data[data_ptr + ci]<<8) | data[data_ptr + ci+1];
					key_count = (key_location & 0xF)+1;
					key_location = i-((key_location>>4)+1);

					while (key_count > 0)
					{
						uncompressed[i] = uncompressed[key_location];
						key_location++;
						key_count--;
						i++;
					}
					ci += 2;
				}
				else
				{
					// do a direct copy
					uncompressed[i] = data[data_ptr + ci];
					ci++;
					i++;
				}

				key >>= 1;
			}
		}
	}
	else
	{
		for (i = 0; i < data_size; i++)
			uncompressed[i] = data[data_ptr + i];
	}

	switch (mapfile)
	{
		case THINGS:
		case LINEDEFS:
		case SIDEDEFS:
		case SEGS:
		case SSECTORS:
		case SECTORS:
	#ifdef WADTYPE_JAG
		case VERTEXES:
		case NODES:
		case REJECT:
		case BLOCKMAP:
	#endif
			converted = (byte *)malloc(data_size);
			for (i = 0; i < data_size; i++)
				converted[i] = uncompressed[i];
			break;
	#ifndef WADTYPE_JAG
		case VERTEXES:
			data_size >>= 1;
			converted = (char *)malloc(data_size);
			for (i = 0; i < data_size; i += 2)
			{
				converted[i] = uncompressed[(i<<1)+1];
				converted[i+1] = uncompressed[(i<<1)];
			}
			break;
		case NODES:
			data_size >>= 1;
			converted = (char *)malloc(data_size);
			for (i = 0; i < data_size; i += 2)
			{
				if ((i % 28) >= 24)
				{
					converted[i] = uncompressed[(i<<1)+3];
					converted[i+1] = uncompressed[(i<<1)+2];
				}
				else
				{
					converted[i] = uncompressed[(i<<1)+1];
					converted[i+1] = uncompressed[(i<<1)];
				}
			}
			break;
		case REJECT:
			converted = (char *)malloc(data_size);
			for (i = 0; i < data_size; i++)
			{
				converted[i] = uncompressed[i];
			}
			break;
		case BLOCKMAP:
			converted = (char *)malloc(data_size);
			for (i = 0; i < data_size; i+= 2)
			{
				converted[i] = uncompressed[i+1];
				converted[i+1] = uncompressed[i];
			}
			break;
	#endif
	}

	fwrite(converted, 1, data_size, out_file);

	WriteTable(lump, out_file_size, data_size);

	out_file_size += data_size;

	free(uncompressed);
	free(converted);
}

void ConvertRawData(int lump)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	fwrite(&data[data_ptr], 1, data_size, out_file);

	WriteTable(lump, out_file_size, data_size);

	out_file_size += data_size;
}

void ConvertFloorData(int lump)
{
	// NO CONVERSION NEEDED! Just do a direct copy!
	ConvertRawData(lump);
}

void ConvertTextureData32X(int lump)
{
	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	short x_size;
	short y_size;
	int offset;

	short x;
	short y;

	byte *output;

	char entryName[9];
	SetEntryName(entryName, (const char*)&table[(lump * 16) + 8]);

	x_size = *(short *)&data[data_ptr];
	y_size = *(short *)&data[data_ptr+2];

	WriteTexture1(x_size, y_size, entryName);

	offset = *(int *)&data[data_ptr+8] + 3;

	output = (byte *)malloc((x_size * y_size) + 320);

	for(x=0; x < x_size; x++){
		for(y=0; y < y_size; y++){
			output[y + (x * y_size)] = data[data_ptr + offset];
			offset++;
		}
		offset += 5;
	}

	for(x=0; x < 320; x++){
		output[(x_size * y_size) + x] = output[x];
	}

	fwrite(output, 1, (x_size * y_size) + 320, out_file);
	WriteTable(lump, out_file_size, (x_size * y_size) + 320);

	out_file_size += ((x_size * y_size) + 320);

	free(output);
}

void ConvertTextureData(int lump)
{
	// write 8 byte header
	// write line index table
	// for each line:
		// write 0x0080
		// copy first byte in line
		// write entire line (including first byte)
		// copy last byte in line
		// write 0xFF
		// repeat these steps for the next line

	int data_ptr = *(int *)&table[(lump*16)] - 0xC;
	int data_size = *(int *)&table[(lump*16)+4];

	byte *output;
	byte *line_lookup;

	int line_lookup_size;

	int array_size = 0;
	short prev_array_size = 0;

	int n;
	int offset;
	int line;

	short x_size;
	short y_size;

	byte texture_name[8];

	int bytes_to_go;
	int bytes_copied;
	int byte_count;

	output = (byte *)malloc(1);

	for (int i = 0; i < 8; i++){
		texture_name[i] = table[(lump*16)+8+i];
		if(texture_name[i] >= 'A' && texture_name[i] <= 'Z')
			texture_name[i] += 0x20; // Set to lowercase
	}

	for (int i = 0; i < texture1_size; i++)
	{
		if (!memcmp(&texture1[i], texture_name, 8))
		{
			// Width
			x_size = (texture1[i + 13] << 8) | texture1[i + 12];
			fwrite(&x_size, 2, 1, out_file);
			// Height
			y_size = (texture1[i + 15] << 8) | texture1[i + 14];
			fwrite(&y_size, 2, 1, out_file);

			i = texture1_size;
		}
	}

	// X offset
	int i = (x_size>>1)-1;
	fwrite(&i, 2, 1, out_file);
	// Y offset
	i = 0x7B;
	fwrite(&i, 2, 1, out_file);

	line_lookup_size = x_size << 2;
	line_lookup = (byte *)malloc(line_lookup_size);

	n = line_lookup_size + 8;
	for (i=0; i < x_size; i++)
	{
		fwrite(&n, 4, 1, out_file);
		n += (y_size+5);
	}

	offset = 0;

	for (line=0; line < x_size; line++)
	{
		fputc(0, out_file);
		fputc(y_size, out_file);

		// duplicate byte
		fputc(data[data_ptr + offset], out_file);

		fwrite(&data[data_ptr + offset], 1, y_size, out_file);

		// duplicate byte
		fputc(data[data_ptr + offset + y_size-1], out_file);

		fputc(0xFF, out_file);

		offset += y_size;
	}

	WriteTable(lump, out_file_size, ((y_size+5)*x_size) + 8 + line_lookup_size);

	out_file_size += (((y_size+5)*x_size) + 8 + line_lookup_size);

	free(line_lookup);
	free(output);
}

void WritePatchName(char *name)
{
	char i;

	if (pnames_table == 0)
	{
		patches_count = 1;
		pnames_table = (byte *)malloc(12);
		*(int *)&pnames_table[0] = 1;
	}
	else
	{
		patches_count++;
		pnames_table = (byte *)realloc(pnames_table, 4 + (patches_count<<3));
		*(int *)&pnames_table[0] = patches_count;
	}

	for (i=0; i<8; i++)
	{
		if(name[i] >= 'a' && name[i] <= 'z')
			name[i] -= 0x20;

		if (name[i] == 0)
		{
			while (i < 8)
			{
				pnames_table[4+((patches_count-1)<<3)+i] = 0;
				i++;
			}
		}
		else
			pnames_table[4+((patches_count-1)<<3)+i] = name[i];
	}
}

void WriteTable(int lump, int ptr, int size)
{
	out_table = (byte *)realloc(out_table, out_table_size + 16);

#ifndef FORCE_LITTLE_ENDIAN
	if (conversion_task == PC_2_MARS){
		ptr = swap_endian(ptr);
		size = swap_endian(size);
	}
#endif

	*(int *)&out_table[out_table_size] = ptr;
	*(int *)&out_table[out_table_size+4] = size;
	out_table[out_table_size+8] = table[(lump*16)+8];
	out_table[out_table_size+9] = table[(lump*16)+9];
	out_table[out_table_size+10] = table[(lump*16)+10];
	out_table[out_table_size+11] = table[(lump*16)+11];
	out_table[out_table_size+12] = table[(lump*16)+12];
	out_table[out_table_size+13] = table[(lump*16)+13];
	out_table[out_table_size+14] = table[(lump*16)+14];
	out_table[out_table_size+15] = table[(lump*16)+15];

	out_table_size += 16;
}

void WriteTableCustom(int ptr, int size, const char *name)
{
	char i;

	out_table = (byte *)realloc(out_table, out_table_size + 16);

#ifndef FORCE_LITTLE_ENDIAN
	if (conversion_task == PC_2_MARS){
		ptr = swap_endian(ptr);
		size = swap_endian(size);
	}
#endif

	*(int *)&out_table[out_table_size] = ptr;
	*(int *)&out_table[out_table_size+4] = size;
	for (i=0; i<8; i++)
	{
		if (name[i] != 0)
			out_table[out_table_size+8+i] = name[i];
		else
		{
			while (i < 8)
			{
				out_table[out_table_size+8+i] = 0;
				i++;
			}
		}
	}

	out_table_size += 16;
}

void WriteTableStart(char type)
{
	out_table = (byte *)realloc(out_table, out_table_size + 16);

	*(int *)&out_table[out_table_size] = 0;
	*(int *)&out_table[out_table_size+4] = 0;

	switch (type)
	{
		case TYPE_TEXTURE:
			if (conversion_task == MARS_2_PC)
				out_table[out_table_size+8] = 'P';
			else	// PC_2_MARS
				out_table[out_table_size+8] = 'T';
			break;
		case TYPE_FLOOR:
			out_table[out_table_size+8] = 'F';
			break;
		case TYPE_SPRITE:
			out_table[out_table_size+8] = 'S';
			break;
	}

	out_table[out_table_size+9] = '_';
	out_table[out_table_size+10] = 'S';
	out_table[out_table_size+11] = 'T';
	out_table[out_table_size+12] = 'A';
	out_table[out_table_size+13] = 'R';
	out_table[out_table_size+14] = 'T';
	out_table[out_table_size+15] = 0;

	out_table_size += 16;
}

void WriteTableEnd(char type)
{
	out_table = (byte *)realloc(out_table, out_table_size + 16);

	*(int *)&out_table[out_table_size] = 0;
	*(int *)&out_table[out_table_size+4] = 0;

	switch(type)
	{
		case TYPE_TEXTURE:
			texture_end_written = 1;
			if(conversion_task == MARS_2_PC)
				out_table[out_table_size+8] = 'P';
			else	// PC_2_MARS
				out_table[out_table_size+8] = 'T';
			break;
		case TYPE_FLOOR:
			floor_end_written = 1;
			out_table[out_table_size+8] = 'F';
			break;
		case TYPE_SPRITE:
			sprite_end_written = 1;
			out_table[out_table_size+8] = 'S';
			break;
	}

	out_table[out_table_size+9] = '_';
	out_table[out_table_size+10] = 'E';
	out_table[out_table_size+11] = 'N';
	out_table[out_table_size+12] = 'D';
	out_table[out_table_size+13] = 0;
	out_table[out_table_size+14] = 0;
	out_table[out_table_size+15] = 0;

	out_table_size += 16;
}

void WriteTexture1(short x_size, short y_size, const char *name)
{
	int i;

	num_of_textures++;

	if (out_texture1_data)
	{
		out_texture1_data = (byte *)realloc(out_texture1_data, num_of_textures << 5);
		out_texture1_table = (byte *)realloc(out_texture1_table, (num_of_textures << 2) + 4);
	}
	else
	{
		out_texture1_data = (byte *)malloc(num_of_textures << 5);
		out_texture1_table = (byte *)malloc((num_of_textures << 2) + 4);
	}

	*(int *)&out_texture1_table[0] = num_of_textures;
	for (i = 0; i < num_of_textures; i++)
		*(int *)&out_texture1_table[(i<<2) + 4] = (num_of_textures << 2) + (i << 5) + 4;

	// Set each entry character to '\0' to start
	memset(&out_texture1_data[(num_of_textures << 5) - 32], 0, 8);

	char lowerEntryName[9];
	strcpy(lowerEntryName, name);
	strlwr(lowerEntryName); // lowercase-ize entry names just to keep with convention

	strcpy((char*)&out_texture1_data[(num_of_textures << 5) - 32], lowerEntryName);
	*(int *)&out_texture1_data[(num_of_textures << 5) - 24] = 0;
	*(short *)&out_texture1_data[(num_of_textures << 5) - 20] = x_size;
	*(short *)&out_texture1_data[(num_of_textures << 5) - 18] = y_size;
	*(int *)&out_texture1_data[(num_of_textures << 5) - 16] = 0;
	*(short *)&out_texture1_data[(num_of_textures << 5) - 12] = 1;
	*(short *)&out_texture1_data[(num_of_textures << 5) - 10] = 0;
	*(short *)&out_texture1_data[(num_of_textures << 5) - 8] = 0;
	//*(short *)&out_texture1_data[(num_of_textures << 5) - 6] = num_of_textures-1;
	*(short *)&out_texture1_data[(num_of_textures << 5) - 4] = 1;
	*(short *)&out_texture1_data[(num_of_textures << 5) - 2] = 0;
}
