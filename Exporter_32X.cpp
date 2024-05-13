#include "Exporter_32X.h"

void Exporter_32X::Execute()
{
}

Exporter_32X::Exporter_32X(WADEntry *entries, FILE *f)
{
	this->entries = entries;
	this->f = f;
}

Exporter_32X::~Exporter_32X()
{
	// Any Cleanup?
}
