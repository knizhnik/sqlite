#include "vdbe-gen.h"

extern int VdbeCompiledQueryCodeQ1(Vdbe *p);
extern char const* const VdbeCompiledQueryTextQ1;

VdbeCompiledQuery const VdbeCompiledQueryCode[] = 
{
	VdbeCompiledQueryCodeQ1,
	NULL
};

char const*const* const VdbeCompiledQueryText[] = 
{
	&VdbeCompiledQueryTextQ1,
	NULL
};



