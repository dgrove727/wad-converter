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
	void *_data;
	size_t _dataLength;
	bool _isCompressed;

public:
	const char *GetName() const;
	void SetName(const char *value);
	const void *GetData() const;
	void SetData(const void *value, size_t length);
	const size_t GetDataLength() const;
	bool IsCompressed() const;
	void SetIsCompressed(bool value);

	WADEntry();
	WADEntry(const char *name, const void *data, size_t length);
	virtual ~WADEntry();
};