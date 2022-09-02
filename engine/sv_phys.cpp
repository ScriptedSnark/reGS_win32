#include "quakedef.h"
#include "sv_phys.h"
#include "progs.h"
#include "server.h"
#include "host.h"

cvar_t sv_gravity = {"sv_gravity", "800", FCVAR_SERVER};

/* My reverse-engineered func
qboolean SV_RunThink(edict_t* ent)
{
	qboolean think; // eax

	if ((ent->v.flags & FENTTABLE_REMOVED) != 0)
	{
		ED_Free(ent);
		return ent->free == false;
	}

	think = true;

	if (ent->v.nextthink > 0.0 && ent->v.nextthink <= host_frametime + sv.time)
	{
		if (sv.time > ent->v.nextthink)
			ent->v.nextthink = sv.time;

		ent->v.nextthink = 0.0;

		gGlobalVariables.time = ent->v.nextthink;
		gEntityInterface.pfnThink(ent);

		if ((ent->v.flags & FENTTABLE_REMOVED) == 0)
			return ent->free == false;

		ED_Free(ent);
		return ent->free == false;
	}

	return think;
}
*/

// Reverse-engineered func from ReHLDS
qboolean SV_RunThink(edict_t* ent)
{
	float thinktime;

	if (!(ent->v.flags & FL_KILLME))
	{
		thinktime = ent->v.nextthink;
		if (thinktime <= 0.0 || thinktime > sv.time + host_frametime)
			return TRUE;

		if (thinktime < sv.time)
			thinktime = sv.time;

		ent->v.nextthink = 0.0f;
		gGlobalVariables.time = thinktime;
		gEntityInterface.pfnThink(ent);
	}

	if (ent->v.flags & FL_KILLME)
	{
		ED_Free(ent);
	}

	return !ent->free;
}

void SV_Physics_None(edict_t* ent)
{
	SV_RunThink(ent);
}
