#pragma once
#include "Listable.h"
#include "common.h"

// List that holds all of the extracted WAD entries in RAM.
// The data from a PC or Jag/32X WAD should be extracted into this list first, then
// the export routines will read the WAD entries from this list and write them back out.
// This will simplify the import/export process and not require multiple file handles to
// be open at the same time, as well as avoid a lot of crazy pointer management.
struct WADEntry : Listable
{
private:
	char *_name;
	uint8_t *_data;
	size_t _dataLength;
	size_t _uncompressedDataLength;
	bool _isCompressed;

public:
	int32_t *dir_entry_filepos; // INTERNAL ONLY FOR USE DURING SAVE PROCESS
	bool pointsToAnotherEntry; // INTERNAL ONY FOR USE DURING SAVE PROCESS

	void SetDataInternal(const uint8_t *value, size_t length);
	const char *GetName() const;
	void SetName(const char *value);
	const uint8_t *GetData() const;
	void SetData(const uint8_t *value, size_t length);
	const size_t GetDataLength() const;
	bool IsCompressed() const;
	void SetIsCompressed(bool value);

	void SetUnCompressedDataLength(size_t value);
	const size_t GetUnCompressedDataLength() const;

	uint8_t *Decompress() const;
	void ReplaceWithFile(const char *filename);
	void DumpToFile(const char *filename) const;

	WADEntry();
	WADEntry(const char *name, const uint8_t *data, size_t length);
	virtual ~WADEntry();

	// Static functions
	static WADEntry *FindEntry(WADEntry *head, const char *name);
	static void PrintList(WADEntry *head);
};