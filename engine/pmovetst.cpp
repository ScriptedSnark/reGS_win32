#include "quakedef.h"
#include "pmovetst.h"
#include "pm_defs.h"
#include <pmove.h>

void PM_GetModelBounds(struct model_s* mod, vec_t* mins, vec_t* maxs)
{
	*mins = mod->mins[0];
	mins[1] = mod->mins[1];
	mins[2] = mod->mins[2];
	*maxs = mod->maxs[0];
	maxs[1] = mod->maxs[1];
	maxs[2] = mod->maxs[2];
}

int PM_GetModelType(struct model_s* mod)
{
	return mod->type;
}

void PM_InitBoxHull()
{
	// TODO: implement - ScriptedSnark
}

int PM_HullPointContents(hull_t* hull, int num, vec_t* p)
{
	// TODO: implement - ScriptedSnark
	return 0;
}

int PM_PointContents(vec_t* p, int* truecontents)
{
	// TODO: implement - Solokiller
	return 0;
}

int PM_WaterEntity(vec_t* p)
{
	// TODO: implement - Solokiller
	return 0;
}

int PM_TruePointContents(vec_t* p)
{
	// TODO: implement - ScriptedSnark
	return 0;
}

float PM_TraceModel(physent_t* pEnt, vec_t* start, vec_t* end, trace_t* trace)
{
	// TODO: implement - ScriptedSnark
	return 0;
}

pmtrace_t* PM_TraceLine(float* start, float* end, int flags, int usehull, int ignore_pe)
{
	// TODO: implement - Solokiller
	return nullptr;
}

pmtrace_t* PM_TraceLineEx(float* start, float* end, int flags, int usehull, int (*pfnIgnore)(physent_t*))
{
	// TODO: implement - ScriptedSnark
	return 0;
}

pmtrace_t PM_PlayerTraceEx(vec_t* start, vec_t* end, int traceFlags, int (*pfnIgnore)(physent_t*))
{
	// TODO: implement - ScriptedSnark
	pmtrace_t trace;

	return trace;
}

int PM_TestPlayerPosition(vec_t* pos, pmtrace_t* ptrace)
{
	// TODO: implement - ScriptedSnark
	return 0;
}

int PM_TestPlayerPositionEx(vec_t* pos, pmtrace_t* ptrace, int (*pfnIgnore)(physent_t*))
{
	// TODO: implement - ScriptedSnark
	return 0;
}

pmtrace_t PM_PlayerTrace(vec_t* start, vec_t* end, int traceFlags, int ignore_pe)
{
	// TODO: implement - Solokiller
	return {};
}
