#pragma once
#include "Exporter.h"
#include <stdio.h>

struct Exporter_Jaguar : Exporter
{
protected:
	FILE *f;
	int wadPtrStart;

public:
	virtual void Execute();

	void SetMaskedInTexture1();

	Exporter_Jaguar(WADEntry *entries, FILE *f, int wadPtrStart);
	virtual ~Exporter_Jaguar();
};