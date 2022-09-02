#include "quakedef.h"
#include "cl_pred.h"
#include "pmove.h"

int pushed, oldphysent, oldvisent;

void CL_PushPMStates()
{
	if (pushed)
	{
		Con_Printf("CL_PushPMStates called with pushed stack\n");
	}
	else
	{
		oldphysent = pmove->numphysent;
		oldvisent = pmove->numvisent;
		pushed = 1;
	}
}

void CL_PopPMStates()
{
	playermove_t* plmove; // edx

	if (!pushed)
	{
		Con_Printf("CL_PopPMStates called without stack\n");
		return;
	}

	if (pushed)
	{
		plmove = pmove;
		pushed--;
		pmove->numphysent = oldphysent;
		plmove->numvisent = oldvisent;
	}
}

void CL_RunUsercmd(local_state_t* from, local_state_t* to, usercmd_t* u, bool runfuncs, double* pfElapsed, unsigned int random_seed)
{
	// TODO: implement - Solokiller
}
