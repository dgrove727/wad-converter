#include "WADEntry.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>

const char *WADEntry::GetName() const
{
	return _name;
}

void WADEntry::SetName(const char *value)
{
	if (_name)
		free(_name);

	_name = strdup(value);
}

const void *WADEntry::GetData() const
{
	return _data;
}

void WADEntry::SetData(const void *value, size_t length)
{
	if (_data)
		free(_data);

	_data = memdup(value, length);
	_dataLength = length;
	_uncompressedDataLength = length;
}

const size_t WADEntry::GetDataLength() const
{
	return _dataLength;
}

bool WADEntry::IsCompressed() const
{
	return _isCompressed;
}

void WADEntry::SetIsCompressed(bool value)
{
	_isCompressed = value;
}

void WADEntry::SetUnCompressedDataLength(size_t value)
{
	_uncompressedDataLength = value;
}

const size_t WADEntry::GetUnCompressedDataLength() const
{
	return _uncompressedDataLength;
}

WADEntry::WADEntry() : Listable()
{
}

WADEntry::WADEntry(const char *name, const void *data, size_t length)
{
	SetName(name);
	SetData(data, length);
}

WADEntry::~WADEntry()
{
	if (_name)
		free(_name);

	if (_data)
		free(_data);
}
