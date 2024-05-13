#include "Exporter_Jaguar.h"

void Exporter_Jaguar::Execute()
{
}

Exporter_Jaguar::Exporter_Jaguar(WADEntry *entries, FILE *f)
{
	this->entries = entries;
	this->f = f;
}

Exporter_Jaguar::~Exporter_Jaguar()
{
	// Any Cleanup?
}
