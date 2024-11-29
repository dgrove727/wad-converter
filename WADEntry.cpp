#include "WADEntry.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include "lzss.h"
#include "CarmackCompress.h"

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

const byte *WADEntry::GetData() const
{
	return _data;
}

void WADEntry::SetDataInternal(const byte *value, size_t length)
{
	if (_data)
		free(_data);

	_data = (byte*)memdup(value, length);
	_dataLength = length;
	_uncompressedDataLength = length;
}

// Sets the data of this WAD entry. If IsCompressed(), then it will
// automatically compress it for you.
void WADEntry::SetData(const byte *value, size_t length)
{
	int32_t filesize = (int32_t)length;

	if (this->IsCompressed())
	{
		int32_t compressedSize = 0;
		byte *recompressFinal = encode(value, filesize, &compressedSize);
		if (compressedSize >= filesize)
		{
			printf("Compressed size is larger or equal to uncompressed size. This is not allowed. Saving as uncompressed.\n");
			this->SetIsCompressed(false);
			this->SetDataInternal(value, filesize);
		}
		else
		{
			this->SetDataInternal(recompressFinal, compressedSize);
			free(recompressFinal);
		}

		this->SetUnCompressedDataLength(filesize);
	}
	else
		this->SetDataInternal(value, filesize);
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

byte *WADEntry::Decompress() const
{
	const uint32_t bufferSize = 0x1000;

	byte *uncompressed = (byte *)malloc(bufferSize);
	lzss_state_t lzss;
	lzss_setup(&lzss, (uint8_t *)this->GetData(), (uint8_t *)uncompressed, bufferSize);

	int uncompSize = this->GetUnCompressedDataLength();

	byte *finalUncompressed = (byte *)malloc(this->GetUnCompressedDataLength());
	byte *writePtr = finalUncompressed;

	while (uncompSize > 0)
	{
		int32_t readSize = uncompSize > bufferSize ? bufferSize : uncompSize;

		lzss_read(&lzss, readSize);
		memcpy(writePtr, lzss.buf, readSize);
		writePtr += readSize;

		uncompSize -= readSize;
	}

	free(uncompressed);

	return finalUncompressed;
}

void WADEntry::ReplaceWithFile(const char *filename)
{
	int32_t filesize;
	byte *newData = ReadAllBytes(filename, &filesize);

	if (this->IsCompressed())
	{
		int compressedSize = 0;
		byte *recompressFinal = encode(newData, filesize, &compressedSize);
		if (compressedSize > filesize)
		{
			printf("Compressed size is larger than uncompressed size. This is not allowed. Saving as uncompressed.\n");
			this->SetIsCompressed(false);
			this->SetData(newData, filesize);
		}
		else
		{
			this->SetData(recompressFinal, compressedSize);
			free(recompressFinal);
		}

		this->SetUnCompressedDataLength(filesize);
	}
	else
		this->SetData(newData, filesize);

	free(newData);
}

void WADEntry::DumpToFile(const char *filename) const
{
	byte *uncompressed = NULL;
	if (this->IsCompressed())
		uncompressed = Decompress();
	WriteAllBytes(filename, uncompressed ? uncompressed : this->GetData(), this->GetUnCompressedDataLength());

	if (uncompressed)
		free(uncompressed);
}

WADEntry::WADEntry() : Listable()
{
}

WADEntry::WADEntry(const char *name, const byte *data, size_t length)
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

WADEntry *WADEntry::FindEntry(WADEntry *head, const char *name)
{
	WADEntry *node;
	for (node = head; node; node = (WADEntry *)node->next)
	{
		if (!strcmp(node->GetName(), name))
			return node;
	}

	return NULL;
}

void WADEntry::PrintList(WADEntry *head)
{
	for (WADEntry *entry = head; entry; entry = (WADEntry *)entry->next)
		printf("%s : %d : %d\n", entry->GetName(), entry->IsCompressed(), entry->GetDataLength());
}