/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"
#include "pmove.h"
#include "pmovetst.h"
#include "tmessage.h"
#include "pr_cmds.h"
#include "cl_ents.h"

vec_t player_mins[4][3] = {
	{
		-16.0f,
		-16.0f,
		-36.0f,
	},
	{
		-16.0f,
		-16.0f,
		-18.0f,
	},
	{
		0.0f,
		0.0f,
		0.0f,
	},
	{
		-32.0f,
		-32.0f,
		-32.0f,
	}};
vec_t player_maxs[4][3] = {
	{
		16.0f,
		16.0f,
		36.0f,
	},
	{
		16.0f,
		16.0f,
		18.0f,
	},
	{
		0.0f,
		0.0f,
		0.0f,
	},
	{
		32.0f,
		32.0f,
		32.0f,
	}};

playermove_t* pmove = nullptr;

movevars_t movevars = {};

hull_t* PM_HullForBsp(physent_t* pe, vec_t* offset)
{
	// TODO: implement - ScriptedSnark
	return 0;
}

void PM_StuckTouch(int hitent, pmtrace_t* ptraceresult)
{
	// TODO: implement - ScriptedSnark
}

void PM_Init(playermove_t* ppm)
{
	PM_InitBoxHull();

	for (int i = 0; i < MAX_MAP_HULLS; i++)
	{
		VectorCopy(player_mins[i], ppm->player_mins[i]);
		VectorCopy(player_maxs[i], ppm->player_maxs[i]);
	}

	ppm->movevars = &movevars;

	ppm->PM_Info_ValueForKey = Info_ValueForKey;
	ppm->PM_Particle = CL_Particle;
	ppm->PM_TestPlayerPosition = PM_TestPlayerPosition;
	ppm->Con_NPrintf = Con_NPrintf;
	ppm->Con_DPrintf = Con_DPrintf;
	ppm->Con_Printf = Con_Printf;
	ppm->Sys_FloatTime = Sys_FloatTime;
	ppm->PM_StuckTouch = PM_StuckTouch;
	ppm->PM_PointContents = PM_PointContents;
	ppm->PM_TruePointContents = PM_TruePointContents;
	ppm->PM_HullPointContents = PM_HullPointContents;
	ppm->PM_PlayerTrace = PM_PlayerTrace;
	ppm->PM_TraceLine = PM_TraceLine;
	ppm->PM_TraceModel = PM_TraceModel;
	ppm->RandomLong = RandomLong;
	ppm->RandomFloat = RandomFloat;
	ppm->PM_GetModelType = PM_GetModelType;
	ppm->PM_HullForBsp = PM_HullForBsp;
	ppm->PM_GetModelBounds = PM_GetModelBounds;
	ppm->COM_FileSize = COM_FileSize;
	ppm->COM_LoadFile = COM_LoadFile;
	ppm->COM_FreeFile = COM_FreeFile;
	ppm->memfgets = memfgets;
	ppm->PM_PlayerTraceEx = PM_PlayerTraceEx;
	ppm->PM_TestPlayerPositionEx = PM_TestPlayerPositionEx;
	ppm->PM_TraceLineEx = PM_TraceLineEx;
}
