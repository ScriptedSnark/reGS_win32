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
#include "server.h"
#include "view.h"
#include "module.h"

vec3_t r_soundOrigin;
vec3_t r_playerViewportAngles;
screenshake_t gVShake;

void V_Init()
{
	// TODO: implement - Solokiller
}

bool V_CheckGamma()
{
	// TODO: implement - Solokiller
	return false;
}

void V_CalcShake()
{
	// TODO: implement - Solokiller
}

void V_ApplyShake(float* origin, float* angles, float factor)
{
	g_engdstAddrs.V_ApplyShake(&origin, &angles, &factor);

	if (origin)
		VectorMA(origin, factor, gVShake.appliedOffset, origin);

	if (angles)
		angles[2] = gVShake.appliedAngle * factor + angles[2];
}

int V_ScreenShake(const char* pszName, int iSize, void* pbuf)
{
	// TODO: implement - Solokiller
	return 0;
}

int V_ScreenFade(const char* pszName, int iSize, void* pbuf)
{
	// TODO: implement - Solokiller
	return 0;
}
