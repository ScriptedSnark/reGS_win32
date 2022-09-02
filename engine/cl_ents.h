#ifndef ENGINE_CL_ENTS_H
#define ENGINE_CL_ENTS_H

#include "pm_defs.h"
#include "cl_entity.h"
#include "client.h"

qboolean CL_IsPlayerIndex(int index);

void CL_ResetFrameStats(frame_t* frame);

void CL_Particle(vec_t* origin, int color, float life, int zpos, int zvel);

void CL_SetSolidPlayers(int playernum);

void CL_SetUpPlayerPrediction(bool dopred, bool bIncludeLocalClient);

void CL_AddStateToEntlist(physent_t* pe, entity_state_t* state);

qboolean CL_EntityTeleported(cl_entity_t* ent);

#endif // ENGINE_CL_ENTS_H
