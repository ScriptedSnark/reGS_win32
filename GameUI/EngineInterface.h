#ifndef GAMEUI_ENGINEINTERFACE_H
#define GAMEUI_ENGINEINTERFACE_H

#include "mathlib.h"
#include "cdll_int.h"
#include "cvardef.h"

#undef ARRAYSIZE
#define ARRAYSIZE(p) (sizeof(p) / sizeof(p[0]))

extern cl_enginefunc_t gEngfuncs;

extern cl_enginefunc_t* engine;

#endif // GAMEUI_ENGINEINTERFACE_H
