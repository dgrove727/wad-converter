#pragma once
#include "Listable.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>

struct SpriteColumn : Listable
{
	const byte *colStart;
	byte length;
	bool duplicate;

	bool Equals(SpriteColumn *checkCol)
	{
		// Easy out
		if (this->length != checkCol->length)
			return false;

		intptr_t addressDelta = checkCol->colStart - this->colStart;
		if (addressDelta < 0 || addressDelta > 65535)
			return false;

		for (int i = 0; i < this->length; i++)
		{
			byte b1 = colStart[i];
			byte b2 = checkCol->colStart[i];

			if (b1 != b2)
				return false;
		}

		return true;
	}

	SpriteColumn(const byte *colStart, byte length)
	{
		this->colStart = colStart;
		this->length = length;
		this->duplicate = false;
	}

	virtual ~SpriteColumn()
	{
	}
};
