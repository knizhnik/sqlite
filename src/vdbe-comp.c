#include "vdbe-gen.h"

extern int VdbeCompiledQueryCodeQ1(Vdbe *p);
extern char const* const VdbeCompiledQueryTextQ1;

extern int VdbeCompiledQueryCodeQ2(Vdbe *p);
extern char const* const VdbeCompiledQueryTextQ2;

VdbeCompiledQuery const VdbeCompiledQueryCode[] = 
{
	VdbeCompiledQueryCodeQ1,
	VdbeCompiledQueryCodeQ2,
	NULL
};

char const*const* const VdbeCompiledQueryText[] = 
{
	&VdbeCompiledQueryTextQ1,
	&VdbeCompiledQueryTextQ2,
	NULL
};



