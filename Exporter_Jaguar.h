#pragma once
#include "Exporter.h"
#include <stdio.h>

struct Exporter_Jaguar : Exporter
{
protected:
	FILE *f;

public:
	virtual void Execute();

	void SetMaskedInTexture1();

	Exporter_Jaguar(WADEntry *entries, FILE *f);
	virtual ~Exporter_Jaguar();
};