#pragma once
#include "Listable.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>

struct FlatList : Listable
{
	char name[16];
	int32_t size;

	FlatList(const char name[9], int32_t size)
	{
		strcpy(this->name, name);
		this->size = size;
	}
	
	virtual ~FlatList()
	{
	}
};
