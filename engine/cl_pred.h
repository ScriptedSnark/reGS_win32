#ifndef ENGINE_CL_PRED_H
#define ENGINE_CL_PRED_H

#include "entity_state.h"
#include "usercmd.h"

extern int pushed, oldphysent, oldvisent;

void CL_PushPMStates();

void CL_PopPMStates();

void CL_RunUsercmd(local_state_t* from, local_state_t* to, usercmd_t* u, bool runfuncs, double* pfElapsed, unsigned int random_seed);

#endif // ENGINE_CL_PRED_H
