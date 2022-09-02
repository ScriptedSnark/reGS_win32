#include "quakedef.h"
#include "cl_ents.h"

qboolean CL_IsPlayerIndex(int index)
{
	if (index > 0)
		return index <= cl.maxclients;

	return false;
}

void CL_ResetFrameStats(frame_t* frame)
{
	frame->clientbytes = 0;
	frame->packetentitybytes = 0;
	frame->tentitybytes = 0;
	frame->playerinfobytes = 0;
	frame->soundbytes = 0;
	frame->usrbytes = 0;
	frame->eventbytes = 0;
	frame->voicebytes = 0;
	frame->msgbytes = 0;
}

void CL_Particle(vec_t* origin, int color, float life, int zpos, int zvel)
{
	// TODO: implement - ScriptedSnark
}

void CL_SetSolidPlayers(int playernum)
{
	// TODO: implement - Solokiller
}

void CL_SetUpPlayerPrediction(bool dopred, bool bIncludeLocalClient)
{
	// TODO: implement - Solokiller
}

void CL_AddStateToEntlist(physent_t* pe, entity_state_t* state)
{
	// TODO: implement - Solokiller
}

qboolean CL_EntityTeleported(cl_entity_t* ent)
{
	if (ent->curstate.origin[0] - ent->prevstate.origin[0] <= 128.0 && ent->curstate.origin[1] - ent->prevstate.origin[1] <= 128.0)
	{
		return ent->curstate.origin[2] - ent->prevstate.origin[2] > 128.0;
	}

	return true;
}
