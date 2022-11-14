#include "quakedef.h"
#include "client.h"

cvar_t ex_interp = { "ex_interp", "0.1", FCVAR_ARCHIVE | FCVAR_FILTERABLE };

void CL_InitExtrap()
{
	Cvar_RegisterVariable(&ex_interp);
}

qboolean CL_FindInterpolationUpdates(cl_entity_t* ent, float targettime, position_history_t** ph0, position_history_t** ph1, int* ph0Index)
{
	// TODO: implement - ScriptedSnark
	return false;
}

void CL_BoundInterpolationFraction(float* frac)
{
	// TODO: implement - ScriptedSnark
}

void CL_UpdatePositions(cl_entity_t* ent)
{
	// TODO: implement - ScriptedSnark
}

void CL_ResetPositions(cl_entity_t* ent)
{
	// TODO: implement - ScriptedSnark
}

void CL_PureOrigin(cl_entity_t* ent, float t, vec_t* outorigin, vec_t* outangles)
{
	// TODO: implement - ScriptedSnark
}

void CL_ComputePlayerOrigin(cl_entity_t* ent)
{
	vec3_t target_origin;
	vec3_t angles;

	if (!ent->player)
		return;

	if (ent->index - 1 == cl.playernum)
		return;

	float t = cl.time - ex_interp.value;

	CL_PureOrigin(ent, t, target_origin, angles);

	VectorCopy(angles, ent->angles);
	VectorCopy(target_origin, ent->origin);
}

void CL_InterpolateEntity(cl_entity_t* ent)
{
	// TODO: implement - ScriptedSnark
}