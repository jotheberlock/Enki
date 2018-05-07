#include "exports.h"

void Exports::addExport(std::string n, FunctionScope * f)
{
    ExportRec rec;
    rec.name = n;
    rec.fun = f;
    recs.push_back(rec);
}