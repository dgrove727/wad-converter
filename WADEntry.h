#pragma once
#include "Listable.h"
#include "common.h"

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