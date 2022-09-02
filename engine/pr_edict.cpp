#include "quakedef.h"
#include "pr_edict.h"
#include "progs.h"
#include "server.h"

void ED_Free(edict_t* ed)
{
	if (!ed->free)
	{
		// SV_UnlinkEdict(ed);
		// FreeEntPrivateData(ed);
		ed->serialnumber++;
		ed->freetime = (float)sv.time;
		ed->free = TRUE;
		ed->v.flags = 0;
		ed->v.model = 0;

		ed->v.takedamage = 0;
		ed->v.modelindex = 0;
		ed->v.colormap = 0;
		ed->v.skin = 0;
		ed->v.frame = 0;
		ed->v.scale = 0;
		ed->v.gravity = 0;
		ed->v.nextthink = -1.0;
		ed->v.solid = SOLID_NOT;

		ed->v.origin[0] = vec3_origin[0];
		ed->v.origin[1] = vec3_origin[1];
		ed->v.origin[2] = vec3_origin[2];
		ed->v.angles[0] = vec3_origin[0];
		ed->v.angles[1] = vec3_origin[1];
		ed->v.angles[2] = vec3_origin[2];
	}
}

void ReleaseEntityDLLFields(edict_t* pEdict)
{
	if (pEdict->pvPrivateData)
	{
		if (gNewDLLFunctions.pfnOnFreeEntPrivateData)
		{
			gNewDLLFunctions.pfnOnFreeEntPrivateData(pEdict);
		}

		Mem_Free(pEdict->pvPrivateData);
	}

	pEdict->pvPrivateData = nullptr;
}
