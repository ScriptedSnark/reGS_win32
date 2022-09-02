#ifndef ENGINE_PMOVETST_H
#define ENGINE_PMOVETST_H

#include "pmtrace.h"
#include "pm_defs.h"

void PM_GetModelBounds(struct model_s* mod, vec_t* mins, vec_t* maxs);

int PM_GetModelType(struct model_s* mod);

void PM_InitBoxHull(void);

int PM_HullPointContents(hull_t* hull, int num, vec_t* p);

int PM_PointContents(vec_t* p, int* truecontents);

int PM_WaterEntity(vec_t* p);

int PM_TruePointContents(vec_t* p);

float PM_TraceModel(physent_t* pEnt, vec_t* start, vec_t* end, trace_t* trace);

pmtrace_t* PM_TraceLine(float* start, float* end, int flags, int usehull, int ignore_pe);

pmtrace_t* PM_TraceLineEx(float* start, float* end, int flags, int usehull, int (*pfnIgnore)(physent_t*));

pmtrace_t PM_PlayerTraceEx(vec_t* start, vec_t* end, int traceFlags, int (*pfnIgnore)(physent_t*));

int PM_TestPlayerPosition(vec_t* pos, pmtrace_t* ptrace);

int PM_TestPlayerPositionEx(vec_t* pos, pmtrace_t* ptrace, int (*pfnIgnore)(physent_t*));

pmtrace_t PM_PlayerTrace(vec_t* start, vec_t* end, int traceFlags, int ignore_pe);

#endif // ENGINE_PMOVETST_H
