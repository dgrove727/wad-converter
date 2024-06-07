#pragma once
#include <stdio.h>
#include "WADEntry.h"

struct Importer_PC
{
protected:
	FILE *in_file;

public:
	virtual WADEntry *Execute();

	Importer_PC(FILE *f);
	virtual ~Importer_PC();
};