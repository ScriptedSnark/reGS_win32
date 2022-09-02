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
/**
 *	@file
 *
 *	model loading and caching
 *
 *	models are the only shared resource between a client and server running
 *	on the same machine.
 */

// Thanks to 1Doomsyaer (https://github.com/1Doomsayer) for helping with complex funcs!

#include "quakedef.h"
#include "qgl.h"
#include "gl_mesh.h"
#include "gl_model.h"
#include <host.h>
#include <client.h>

model_t* loadmodel;
char loadname[32];

struct mod_known_info_t
{
	qboolean shouldCRC;	   // Needs a CRC check?
	qboolean firstCRCDone; // True when the first CRC check has been finished

	CRC32_t initialCRC;
};

mod_known_info_t mod_known_info[MAX_MOD_KNOWN];

byte* mod_base = nullptr;

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

aliashdr_t* pheader;

stvert_t stverts[MAXALIASVERTS];
mtriangle_t triangles[MAXALIASTRIS];

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
trivertx_t* poseverts[MAXALIASFRAMES];
int posenum;

char* wadpath = nullptr;
int mod_numknown;
model_t mod_known[1024];

void* Mod_Extradata(model_t* mod)
{
	void* result;

	if (!mod)
		return nullptr;

	result = Cache_Check(&mod->cache);

	if (result)
		return result;

	Mod_LoadModel(mod, true, false);

	if (!mod->cache.data)
		Sys_Error("Mod_Extradata: caching failed");

	return mod->cache.data;
}

void Mod_ClearAll()
{
	int i;
	model_t* mod;

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (mod->type != mod_alias && mod->needload != (NL_NEEDS_LOADED | NL_UNREFERENCED))
		{
			mod->needload = NL_UNREFERENCED;

			if (mod->type == mod_sprite)
				mod->cache.data = nullptr;
		}
	}
}

void Mod_FillInCRCInfo(bool trackCRC, int model_number)
{
	mod_known_info[model_number].firstCRCDone = false;
	mod_known_info[model_number].shouldCRC = trackCRC;
	mod_known_info[model_number].initialCRC = 0;
}

model_t* Mod_FindName(bool trackCRC, const char* name)
{
	int i;
	model_t *mod, *avail = nullptr;

	if (!name[0])
		Sys_Error("Mod_FindName: NULL name");

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!Q_stricmp(mod->name, name))
			break;

		if (mod->needload == NL_UNREFERENCED)
		{
			if (!avail || mod->type != mod_alias && mod->type != mod_studio)
				avail = mod;
		}
	}

	if (i == mod_numknown)
	{
		if (mod_numknown < MAX_MOD_KNOWN)
		{
			Mod_FillInCRCInfo(trackCRC, mod_numknown);
			mod_numknown++;
		}
		else
		{
			if (!avail)
				Sys_Error("Mod_FindName: mod_numknown >= MAX_KNOWN_MODELS");

			mod = avail;
			Mod_FillInCRCInfo(trackCRC, avail - mod_known);
		}

		Q_strncpy(mod->name, name, sizeof(mod->name) - 1);
		mod->name[sizeof(mod->name) - 1] = 0;

		if (mod->needload != (NL_NEEDS_LOADED | NL_UNREFERENCED))
			mod->needload = NL_NEEDS_LOADED;
	}

	return mod;
}

bool Mod_ValidateCRC(const char* name, CRC32_t crc)
{
	model_t* mod = Mod_FindName(true, name);
	mod_known_info_t* p = &mod_known_info[mod - mod_known];

	if (p->firstCRCDone)
	{
		if (p->initialCRC != crc)
			return false;
	}

	return true;
}

void Mod_NeedCRC(const char* name, bool needCRC)
{
	model_t* mod = Mod_FindName(false, name);
	mod_known_info_t* p = &mod_known_info[mod - mod_known];

	p->shouldCRC = needCRC;
}

void Mod_ChangeGame(void)
{
	int i;
	model_t* mod = nullptr;
	mod_known_info_t* p = nullptr;

	for (i = 0; i < mod_numknown; i++)
	{
		mod = &mod_known[i];

		if (mod->type == mod_studio)
		{
			if (Cache_Check(&mod->cache))
				Cache_Free(&mod->cache);
		}

		p = &mod_known_info[i];
		p->firstCRCDone = false;
		p->initialCRC = 0;
	}
}

model_t* Mod_LoadModel(model_t* mod, const bool crash, const bool trackCRC)
{
	CRC32_t currentCRC;
	char tmpName[MAX_PATH];

	FileHandle_t file;

	byte* buf = nullptr;
	mod_known_info_t* pinfo = nullptr;

	if (mod->type == mod_alias || mod->type == mod_studio)
	{
		if (Cache_Check(&mod->cache))
		{
			mod->needload = NL_PRESENT;
			return mod;
		}
	}
	else
	{
		if (mod->needload == NL_PRESENT || mod->needload == (NL_NEEDS_LOADED | NL_UNREFERENCED))
			return mod;
	}

	// Check for -steam launch param
	if (COM_CheckParm("-steam") && mod->name[0] == '/')
	{
		char* p = mod->name;
		while (*(++p) == '/')
			;

		strncpy(tmpName, p, sizeof(tmpName) - 1);
		tmpName[sizeof(tmpName) - 1] = 0;

		strncpy(mod->name, tmpName, sizeof(mod->name) - 1);
		mod->name[sizeof(mod->name) - 1] = 0;
	}

	//
	// Load the file, check if it exists
	//
	file = FS_Open(mod->name, "rb");

	if (!file)
	{
		if (crash || developer.value > 1.0)
			Sys_Error("Mod_NumForName: %s not found", mod->name);

		Con_Printf("Error: could not load file %s\n", mod->name);
		return nullptr;
	}

	int length;
	buf = (byte*)FS_GetReadBuffer(file, &length);

	if (buf)
	{
		if (LittleLong(*(uint32*)buf) == 'TSDI')
		{
			FS_ReleaseReadBuffer(file, buf);
			buf = nullptr;
		}
	}

	if (!buf)
	{
		length = FS_Size(file);
		buf = (byte*)Hunk_TempAlloc(length + 1);
		FS_Read(buf, length, file);
		FS_Close(file);

		if (!buf)
		{
			if (crash || developer.value > 1.0)
				Sys_Error("Mod_NumForName: %s not found", mod->name);

			Con_Printf("Error: could not load file %s\n", mod->name);
			return nullptr;
		}

		file = 0;
	}

	if (trackCRC)
	{
		pinfo = &mod_known_info[mod - mod_known];

		if (pinfo->shouldCRC)
		{
			CRC32_Init(&currentCRC);
			CRC32_ProcessBuffer(&currentCRC, buf, length);
			currentCRC = CRC32_Final(currentCRC);

			if (pinfo->firstCRCDone)
			{
				if (currentCRC != pinfo->initialCRC)
					Sys_Error("%s has been modified since starting the engine.  Consider running system diagnostics to check for faulty hardware.\n", mod->name);
			}
			else
			{
				pinfo->firstCRCDone = true;
				pinfo->initialCRC = currentCRC;

				SetCStrikeFlags();

				if (!IsGameSubscribed("czero") && g_bIsCStrike /* && IsCZPlayerModel(currentCRC, mod->name)*/ && cls.state != ca_dedicated)
				{
					COM_ExplainDisconnection(true, "Cannot continue with altered model %s, disconnecting.", mod->name);
					// CL_Disconnect();
					return nullptr;
				}
			}
		}
	}

	if (developer.value > 1.0)
		Con_DPrintf("loading %s\n", mod->name);

	//
	// allocate a new model
	//
	COM_FileBase(mod->name, loadname);

	loadmodel = mod;

	//
	// fill it in
	//

	// call the apropriate loader
	mod->needload = NL_PRESENT;

	/* - TODO: implement - ScriptedSnark
	switch (LittleLong(*(unsigned*)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadAliasModel(mod, buf);
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel(mod, buf);
		break;

	case IDSTUDIOHEADER:
		Mod_LoadStudioModel(mod, buf);
		break;

	default:
		DT_LoadDetailMapFile(loadname);
		Mod_LoadBrushModel(mod, buf);
		break;
	}
	*/
	if (file)
	{
		FS_ReleaseReadBuffer(file, buf);
		FS_Close(file);
	}

	return mod;
}

model_t* Mod_ForName(const char* name, bool crash, bool trackCRC)
{
	model_t* mod;
	mod = Mod_FindName(trackCRC, name);

	if (!mod)
		return nullptr;

	return Mod_LoadModel(mod, crash, trackCRC);
}

void Mod_MarkClient(model_t* pModel)
{
	pModel->needload = (NL_NEEDS_LOADED | NL_UNREFERENCED);
}

void Mod_LoadEntities(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->entities = nullptr;
		return;
	}

	loadmodel->entities = (char*)Hunk_AllocName(l->filelen, loadname);
	Q_memcpy(loadmodel->entities, mod_base + l->fileofs, l->filelen);

	if (loadmodel->entities)
	{
		char* pszInputStream = COM_Parse(loadmodel->entities);

		if (*pszInputStream)
		{
			while (com_token[0] != '}')
			{
				if (!Q_strcmp(com_token, "wad"))
				{
					COM_Parse(pszInputStream);

					if (wadpath)
						Mem_Free(wadpath);

					wadpath = Mem_Strdup(com_token);
					return;
				}

				pszInputStream = COM_Parse(pszInputStream);

				if (!*pszInputStream)
					return;
			}
		}
	}
}

void Mod_LoadLighting(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = nullptr;
		return;
	}

	loadmodel->lightdata = (color24*)Hunk_AllocName(l->filelen, loadname);
	Q_memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}

void Mod_LoadVisibility(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->visdata = nullptr;
		return;
	}

	loadmodel->visdata = (byte*)Hunk_AllocName(l->filelen, loadname);
	Q_memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}

void Mod_LoadVertexes(lump_t* l)
{
	dvertex_t* in;
	mvertex_t* out;
	int i, count;

	in = (dvertex_t*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mvertex_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

void Mod_LoadSubmodels(lump_t* l)
{
	dmodel_t* in;
	dmodel_t* out;
	int i, j, count;
	in = (dmodel_t*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (dmodel_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}

		for (j = 0; j < MAX_MAP_HULLS; j++)
			out->headnode[j] = LittleLong(in->headnode[j]);

		out->visleafs = LittleLong(in->visleafs);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

void Mod_LoadEdges(lump_t* l)
{
	dedge_t* in;
	medge_t* out;
	int i, count;

	in = (dedge_t*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (medge_t*)Hunk_AllocName((count + 1) * sizeof(*out), loadname);

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

void CalcSurfaceExtents(msurface_t* s)
{
	float mins[2], maxs[2], val;
	int i, j, e;
	mvertex_t* v;
	mtexinfo_t* tex;
	int bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;
	tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		e = loadmodel->surfedges[s->firstedge + i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j = 0; j < 2; j++)
		{
			val = v->position[0] * (double)tex->vecs[j][0] +
				  v->position[1] * (double)tex->vecs[j][1] +
				  v->position[2] * (double)tex->vecs[j][2] +
				  (double)tex->vecs[j][3];

			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++)
	{
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

		if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > 512)
		{
			Sys_Error("Bad surface extents %d/%d at position (%d,%d,%d)", s->extents[0], s->extents[1],
				(int)v->position[0], (int)v->position[1], (int)v->position[2]);
		}
	}
}

void Mod_SetParent(mnode_t* node, mnode_t* parent)
{
	node->parent = parent;

	if (node->contents < 0)
		return;

	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

void Mod_LoadNodes(lump_t* l)
{
	int i, j, count, p;
	dnode_t* in;
	mnode_t* out;
	in = (dnode_t*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(dnode_t))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mnode_t*)Hunk_AllocName(count * sizeof(*out), loadname);
	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;
		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);

		for (j = 0; j < 2; j++)
		{
			p = LittleShort(in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent(loadmodel->nodes, nullptr);
}

void Mod_LoadLeafs(lump_t* l)
{
	dleaf_t* in;
	mleaf_t* out;
	int i, j, count, p;

	in = (dleaf_t*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mleaf_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;
		out->firstmarksurface = loadmodel->marksurfaces + LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);
		p = LittleLong(in->visofs);

		if (p == -1)
		{
			out->compressed_vis = nullptr;
		}
		else
		{
			out->compressed_vis = loadmodel->visdata + p;
		}

		out->efrags = nullptr;

		for (j = 0; j < 4; j++)
		{
			out->ambient_sound_level[j] = in->ambient_level[j];
		}
	}
}

void Mod_LoadClipnodes(lump_t* l)
{
	dclipnode_t *in, *out;
	int i, count;
	hull_t* hull;

	in = (dclipnode_t*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (dclipnode_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;
	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -36;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 36;
	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -32;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 32;
	hull = &loadmodel->hulls[3];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -18;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 18;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

void Mod_MakeHull0(void)
{
	mnode_t *in, *child;
	dclipnode_t* out;
	int i, j, count;
	hull_t* hull;

	hull = &loadmodel->hulls[0];

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = (dclipnode_t*)Hunk_AllocName(count * sizeof(*out), loadname);

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j = 0; j < 2; j++)
		{
			child = in->children[j];

			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

void Mod_LoadMarksurfaces(lump_t* l)
{
	int i, j, count;
	short* in;
	msurface_t** out;

	in = (short*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (msurface_t**)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (i = 0; i < count; i++)
	{
		j = LittleShort(in[i]);

		if (j >= loadmodel->numsurfaces)
			Sys_Error("Mod_ParseMarksurfaces: bad surface number");

		out[i] = loadmodel->surfaces + j;
	}
}

void Mod_LoadSurfedges(lump_t* l)
{
	int i, count;
	int *in, *out;
	in = (int*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (int*)Hunk_AllocName(count * sizeof(*out), loadname);

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (i = 0; i < count; i++)
		out[i] = LittleLong(in[i]);
}

void Mod_LoadPlanes(lump_t* l)
{
	int i, j;
	mplane_t* out;
	dplane_t* in;
	int count;
	int bits;

	in = (dplane_t*)(mod_base + l->fileofs);

	if (l->filelen % sizeof(*in))
		Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);

	count = l->filelen / sizeof(*in);
	out = (mplane_t*)Hunk_AllocName(count * 2 * sizeof(*out), loadname);

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;

		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);

			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}

float RadiusFromBounds(vec_t* mins, vec_t* maxs)
{
	int i;
	vec3_t corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length(corner);
}

void Mod_LoadBrushModel(model_t* mod, void* buffer)
{
	int i, j, k;
	dheader_t* header;
	dmodel_t* bm;

	loadmodel->type = mod_brush;

	header = (dheader_t*)buffer;

	i = LittleLong(header->version);

	if (i != Q1BSP_VERSION && i != BSPVERSION)
	{
		if (cls.state != ca_dedicated)
		{
			COM_ExplainDisconnection(true, "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)\n", mod, i, BSPVERSION);
			// CL_Disconnect();
		}
		return;
	}

	mod_base = (byte*)buffer;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int*)header)[i] = LittleLong(((int*)header)[i]);

	Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);

	if (!Q_stricmp(com_gamedir, "bshift"))
	{
		Mod_LoadEntities(&header->lumps[LUMP_ENTITIES + 1]);
		// Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
		Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
		Mod_LoadPlanes(&header->lumps[LUMP_PLANES - 1]);
	}
	else
	{
		Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
		// Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
		Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
		Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
	}

	// Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
	// Mod_LoadFaces(&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES]);
	Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes(&header->lumps[LUMP_NODES]);
	Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES]);
	Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);

	for (k = 0; k < mod->numsurfaces; k++)
	{
		if (mod->surfaces[k].numedges + mod->surfaces[k].firstedge > mod->numsurfedges)
			Sys_Error("MOD_LoadBrushModel: more surfedges referenced than loaded in %s", mod->name);
	}

	for (k = 0; k < mod->numleafs; k++)
	{
		if (mod->leafs[k].nummarksurfaces + mod->leafs[k].firstmarksurface - mod->marksurfaces > mod->nummarksurfaces)
			Sys_Error("MOD_LoadBrushModel: more marksurfaces referenced than loaded in %s", mod->name);
	}

	for (k = 0; k < mod->numnodes; k++)
	{
		if (mod->nodes[k].firstsurface + mod->nodes[k].numsurfaces > mod->numsurfaces)
			Sys_Error("MOD_LoadBrushModel: more surfaces referenced than loaded in %s", mod->name);
	}

	for (k = 0; k < mod->numsubmodels; k++)
	{
		if (mod->submodels[k].firstface + mod->submodels[k].numfaces > mod->numsurfaces)
			Sys_Error("MOD_LoadBrushModel: more faces referenced than loaded in %s", mod->name);
	}

	Mod_MakeHull0();
	mod->numframes = 2;
	mod->flags = 0;

	for (i = 0; i < mod->numsubmodels; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j = 1; j < MAX_MAP_HULLS; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;
		mod->maxs[0] = bm->maxs[0];
		mod->maxs[2] = bm->maxs[2];
		mod->maxs[1] = bm->maxs[1];
		mod->mins[0] = bm->mins[0];
		mod->mins[1] = bm->mins[1];
		mod->mins[2] = bm->mins[2];
		mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels - 1)
		{
			char name[10];

			snprintf(name, sizeof(name), "*%i", i + 1);
			loadmodel = Mod_FindName(false, name);
			*loadmodel = *mod;
			Q_strncpy(loadmodel->name, name, sizeof(loadmodel->name) - 1);
			loadmodel->name[sizeof(loadmodel->name) - 1] = 0;

			mod = loadmodel;
		}
	}
}

void Mod_SpriteTextureName(char* pszName, int nNameSize, const char* pcszModelName, int framenum)
{
	snprintf(pszName, nNameSize, "%s_%i", pcszModelName, framenum);
}

void Mod_UnloadSpriteTextures(model_t* pModel)
{
	int i;
	char name[256];
	msprite_t* spr = nullptr;

	if (!pModel)
		return;

	if (pModel->type != mod_sprite)
		return;

	pModel->needload = NL_NEEDS_LOADED;

	spr = (msprite_t*)pModel->cache.data;

	if (spr)
	{
		for (i = 0; i < spr->numframes; i++)
		{
			sprintf(name, "%s_%i", pModel->name, i);
			// GL_UnloadTexture(name); - TODO: implement - ScriptedSnark
		}
	}
}

mleaf_t* Mod_PointInLeaf(vec_t* p, model_t* model)
{
	mnode_t* node;
	float d;
	mplane_t* plane;

	if (!model || !model->nodes)
		Sys_Error("Mod_PointInLeaf: bad model");

	node = model->nodes;

	while (true)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;

		plane = node->plane;

		d = DotProduct(p, plane->normal) - plane->dist;

		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return nullptr; // never reached
}

void Mod_Print(void)
{
	int i;
	model_t* mod;

	Con_Printf("Cached models:\n");

	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		Con_Printf("%8p : %s", mod->cache.data, mod->name);

		if (mod->needload & NL_UNREFERENCED)
			Con_Printf(" (!R)");
		if (mod->needload & NL_NEEDS_LOADED)
			Con_Printf(" (!P)");

		Con_Printf("\n");
	}
}
