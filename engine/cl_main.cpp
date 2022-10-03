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
#include "client.h"
#include "cl_main.h"
#include "cl_parse.h"
#include "cl_spectator.h"
#include "cl_tent.h"
#include "com_custom.h"
#include "gl_rmain.h"
#include "hashpak.h"
#include "host.h"
#include "pmove.h"
#include "tmessage.h"
#include "gl_vidnt.h"
#include "vgui_int.h"
#include "host_cmd.h"
#include "cl_servercache.h"
#include "net.h"
#include "winsani_in.h"
#include <winsock.h>
#include "winsani_out.h"

int num_servers;
server_cache_t cached_servers[16];
int msg_buckets[64];
int total_data[64];
netadr_t g_GameServerAddress;
netadr_t net_local_adr;

client_static_t cls;
client_state_t cl;

cl_entity_t* cl_entities = nullptr;

// TODO, allocate dynamically
efrag_t cl_efrags[MAX_EFRAGS] = {};
dlight_t cl_dlights[MAX_DLIGHTS] = {};
dlight_t cl_elights[MAX_ELIGHTS] = {};
lightstyle_t cl_lightstyle[MAX_LIGHTSTYLES] = {};

// TODO: implement API and add here - Solokiller
playermove_t g_clmove;


float g_LastScreenUpdateTime = 0;

cvar_t cl_weaponlistfix = {"cl_weaponlistfix", "0"};
cvar_t cl_fixtimerate = {"cl_fixtimerate", "7.5"};
cvar_t cl_showmessages = {"cl_showmessages", "0"};
cvar_t cl_connect_test = {"cl_connect_test", "0"};
cvar_t cl_needinstanced = {"cl_needinstanced", "0"};
cvar_t cl_showfps = {"cl_showfps", "0", FCVAR_ARCHIVE};
cvar_t cl_cmdbackup = {"cl_cmdbackup", "2", FCVAR_ARCHIVE};
cvar_t cl_showevents = {"cl_showevents", "0"};
cvar_t cl_clockreset = {"cl_clockreset", "0.1"};
cvar_t cl_himodels = {"cl_himodels", "0", FCVAR_ARCHIVE};
cvar_t cl_gaitestimation = {"cl_gaitestimation", "1"};
cvar_t cl_timeout = {"cl_timeout", "60", FCVAR_ARCHIVE};
cvar_t cl_shownet = {"cl_shownet", "0"};
cvar_t cl_showsizes = {"cl_showsizes", "0", FCVAR_SPONLY};
cvar_t cl_solid_players = {"cl_solid_players", "1"};
cvar_t cl_nodelta = {"cl_nodelta", "0"};
cvar_t cl_idealpitchscale = {"cl_idealpitchscale", "0.8", FCVAR_ARCHIVE};
cvar_t rcon_address = {"rcon_address", "", FCVAR_PRIVILEGED};
cvar_t rcon_port = {"rcon_port", "0", FCVAR_PRIVILEGED};
cvar_t cl_resend = {"cl_resend", "6.0"};
cvar_t cl_slisttimeout = {"cl_slist", "10.0"};
cvar_t cl_allow_download = {"cl_allowdownload", "1", FCVAR_ARCHIVE};
cvar_t cl_allow_upload = {"cl_allowupload", "1", FCVAR_ARCHIVE};
cvar_t cl_cmdrate = {"cl_cmdrate", "60", FCVAR_ARCHIVE};
cvar_t cl_download_ingame = {"cl_download_ingame", "1", FCVAR_ARCHIVE};
cvar_t cl_logofile = {"cl_logofile", "lambda", FCVAR_ARCHIVE};
cvar_t cl_logocolor = {"cl_logocolor", "orange", FCVAR_ARCHIVE};
cvar_t password = {"password", "", FCVAR_USERINFO};
cvar_t cl_name = {"name", "unnamed", FCVAR_ARCHIVE | FCVAR_USERINFO | FCVAR_PRINTABLEONLY | FCVAR_NOEXTRAWHITEPACE};
cvar_t team = {"team", "", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t skin = {"skin", "", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t cl_model = {"model", "", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t topcolor = {"topcolor", "0", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t bottomcolor = {"bottomcolor", "0", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t rate = {"rate", "30000", FCVAR_ARCHIVE | FCVAR_USERINFO | FCVAR_FILTERABLE};
cvar_t cl_updaterate = {"cl_updaterate", "60", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t cl_lw = {"cl_lw", "1", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t cl_lc = {"cl_lc", "1", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t cl_dlmax = {"cl_dlmax", "512", FCVAR_ARCHIVE | FCVAR_USERINFO};
cvar_t fs_lazy_precache = {"fs_lazy_precache", "0"};
cvar_t fs_precache_timings = {"fs_precache_timings", "0"};
cvar_t fs_perf_warnings = {"fs_perf_warnings", "0"};
cvar_t fs_startup_timings = {"fs_startup_timings", "0"};
cvar_t cl_mousegrab = {"cl_mousegrab", "1", FCVAR_ARCHIVE};
cvar_t m_rawinput = {"m_rawinput", "0", FCVAR_ARCHIVE};
cvar_t cl_filterstuffcmd = {"cl_filterstuffcmd", "0", FCVAR_ARCHIVE | FCVAR_PRIVILEGED};

static int g_iCurrentTiming = 0;

startup_timing_t g_StartupTimings[MAX_STARTUP_TIMINGS] = {};

void SetupStartupTimings()
{
	g_iCurrentTiming = 0;
	g_StartupTimings[g_iCurrentTiming].name = "Startup";
	g_StartupTimings[g_iCurrentTiming].time = Sys_FloatTime();
}

void AddStartupTiming(const char* name)
{
	++g_iCurrentTiming;
	g_StartupTimings[g_iCurrentTiming].name = name;
	g_StartupTimings[g_iCurrentTiming].time = Sys_FloatTime();
}

void PrintStartupTimings()
{
	Con_Printf("Startup timings (%.2f total)\n", g_StartupTimings[g_iCurrentTiming].time - g_StartupTimings[0].time);
	Con_Printf("    0.00    Startup\n");

	// Print the relative time between each system startup
	for (int i = 1; i < g_iCurrentTiming; ++i)
	{
		Con_Printf("    %.2f    %s\n",
			g_StartupTimings[i].time - g_StartupTimings[i - 1].time,
			g_StartupTimings[i].name);
	}
}

void CL_CheckClientState(void)
{
	if ((cls.state == ca_connected || cls.state == ca_uninitialized) &&
		cls.signon == SIGNONS)
	{
		// first update is the final signon stage
		cls.state = ca_active;

		if (fs_startup_timings.value != 0.0)
			PrintStartupTimings();

		{
			unsigned char* IPPtr = cls.netchan.remote_address.ip;
			unsigned short port = cls.netchan.remote_address.port;

			if (!port)
			{
				IPPtr = net_local_adr.ip;
				port = net_local_adr.port;
			}

			VGuiWrap2_NotifyOfServerConnect(com_gamedir, *((int*)IPPtr), /*Q_*/ ntohs(port));
		}
	}
}

void CL_Connect_f(void) // This function crashes the game due to exception in client.dll (TODO: FIX - ScriptedSnark)
{
	char name[MAX_PATH];
	CareerStateType prevcareer = CAREER_NONE;

	const char* args = nullptr;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("Usage:  connect <server>\n");
		return;
	}

	SetCStrikeFlags();

	if ((g_bIsCStrike || g_bIsCZero) && cmd_source != src_command)
	{
		Con_Printf("Connect only works from the console.\n");
		return;
	}

	args = Cmd_Args();

	if (!args)
		return;

	Q_strncpy(name, args, sizeof(name));
	name[sizeof(name) - 1] = 0;

	/* - TODO: implement demo playback - ScriptedSnark
	if (cls.demoplayback)
		CL_StopPlayback();
	*/

	prevcareer = g_careerState;

	// CL_Disconnect(); - TODO: implement - ScriptedSnark

	if (prevcareer == CAREER_LOADING)
		g_careerState = CAREER_LOADING;

	StartLoadingProgressBar("Connecting", 12);
	SetLoadingProgressBarStatusText("#GameUI_EstablishingConnection");

	int num = Q_atoi(name); // In case it's an index.

	if ((num > 0) && (num <= num_servers) && !Q_strstr(cls.servername, "."))
	{
		Q_strncpy(name, NET_AdrToString(cached_servers[num - 1].adr), sizeof(name));
		name[sizeof(name) - 1] = 0;
	}

	Q_memset(msg_buckets, 0, sizeof(msg_buckets));
	Q_memset(total_data, 0, sizeof(total_data));

	Q_strncpy(cls.servername, name, sizeof(cls.servername) - 1);
	cls.servername[sizeof(cls.servername) - 1] = 0;

	cls.state = ca_connecting;
	cls.connect_time = -99999;

	cls.connect_retry = 0;

	cls.passive = false;
	cls.spectator = false;
	cls.isVAC2Secure = false;

	cls.GameServerSteamID = 0;

	cls.build_num = 0;

	Q_memset(&g_GameServerAddress, 0, sizeof(netadr_t));
	cls.challenge = 0;

	gfExtendedError = false;
	g_LastScreenUpdateTime = 0.0;

	if (Q_strnicmp(cls.servername, "local", 5))
	{
		// allow remote
		NET_Config(true);
	}
}

void CL_Retry_f() // TODO: improve - ScriptedSnark
{
	char szCommand[260]; // [esp+1Ch] [ebp-110h] BYREF

	if (cls.servername[0])
	{
		if (strchr(cls.servername, 10) || strchr(cls.servername, 59))
			Con_Printf("Invalid command separator in server name, refusing retry\n");
		else
		{
			if (cls.passive)
				snprintf(szCommand, sizeof(szCommand), "listen %s\n", cls.servername);
			else
				snprintf(szCommand, sizeof(szCommand), "connect %s\n", cls.servername);

			Cbuf_AddText(szCommand);
			Con_Printf("Commencing connection retry to %s\n", cls.servername);
		}
	}
	else
	{
		Con_Printf("Can't retry, no previous connection\n");
	}
}

void CL_TakeSnapshot_f()
{
	model_s* in;
	int i;
	FileHandle_t file;
	char base[64];
	char filename[64];

	if (cl.num_entities && (in = cl_entities->model) != 0)
		COM_FileBase(in->name, base);
	else
		Q_strcpy(base, "Snapshot");

	for (i = 0; i != 1000; i++)
	{
		Q_snprintf(filename, sizeof(filename), "%s%04d.bmp", base, i);
		file = FS_OpenPathID(filename, "r", "GAMECONFIG");

		if (!file)
		{
			VID_TakeSnapshot(filename);
			return;
		}

		FS_Close(file);
	}

	Con_Printf("Unable to take a screenshot.\n");
}

void CL_ShutDownClientStatic() // probably need to improve if doesn't work
{
	int client = CL_UPDATE_BACKUP;
	int i;
	packet_entities_t* ent;

	for (i = 0; i < CL_UPDATE_BACKUP; i++)
	{
		ent = &cl.frames[i].packet_entities;
		if (ent->entities)
		{
			Mem_Free(ent->entities);
		}
		ent->entities = NULL;
	}

	Q_memset(cl.frames, 0, sizeof(frame_t) * client);
}

void CL_Shutdown()
{
	// TODO: implement - Solokiller
	TextMessageShutdown();
	// TODO: implement - Solokiller
}

void CL_Init()
{
	CL_InitTEnts();
	// TODO: implement - Solokiller
	TextMessageInit();
	// TODO: implement - Solokiller
	Cmd_AddCommand /*WithFlags*/ ("connect", CL_Connect_f /*, 8*/); // TODO: implement slowhacking protection - ScriptedSnark
	Cmd_AddCommand /*WithFlags*/ ("retry", CL_Retry_f /*, 8*/);		// TODO: also uncomment when this new func from GoldSrc update will be finished - ScriptedSnark
	Cmd_AddCommand("snapshot", CL_TakeSnapshot_f);
	Cvar_RegisterVariable(&rate);
	Cvar_RegisterVariable(&cl_lw);
	Cvar_RegisterVariable(&cl_showfps);
	Cvar_RegisterVariable(&dev_overview);
	Cvar_RegisterVariable(&cl_mousegrab);
	Cvar_RegisterVariable(&m_rawinput);
	// TODO: implement - Solokiller
}

dlight_t* CL_AllocDlight(int key)
{
	// TODO: implement - Solokiller
	return nullptr;
}

dlight_t* CL_AllocElight(int key) // - TODO: improve - ScriptedSnark
{
	dlight_t* dest; // ebx

	if (key)
	{
		dest = cl_elights;

		while (dest->key != key)
		{
			if (dest++ == &cl_elights[64])
				goto Alloc;
		}
	}
	else
	{

	Alloc:
		dest = cl_elights;
		while (cl.time <= (long double)dest->die)
		{
			if (dest++ == &cl_elights[64])
			{
				Q_memset(cl_elights, 0, 40);
				cl_elights[0].key = key;
				return cl_elights;
			}
		}
	}

	Q_memset(dest, 0, 40);
	dest->key = key;
	return dest;
}

model_t* CL_GetModelByIndex(int index)
{
	if (index >= MAX_MODELS)
		return nullptr;

	model_t* model = cl.model_precache[index]; // ebx

	if (!model)
		return nullptr;

	if (model->needload == NL_NEEDS_LOADED || model->needload == NL_UNREFERENCED)
	{
		if (fs_precache_timings.value == 0.0)
		{
			Mod_LoadModel(model, false, false);
		}
		else
		{
			double start = Sys_FloatTime();
			Mod_LoadModel(model, false, false);
			double end = Sys_FloatTime() - start;
			Con_DPrintf("fs_precache_timings: loaded model %s in time %.3f sec\n", model->name, end);
		}
	}

	return model;
}

void CL_GetPlayerHulls()
{
	for (int i = 0; i < 4; ++i)
	{
		if (!ClientDLL_GetHullBounds(i, player_mins[i], player_maxs[i]))
			break;
	}
}

bool UserIsConnectedOnLoopback()
{
	return cls.netchan.remote_address.type == NA_LOOPBACK;
}

void CL_HudMessage(const char* pMessage)
{
	DispatchDirectUserMsg("HudText", Q_strlen(pMessage), (void*)pMessage);
}

void SetPal(int i)
{
	// Nothing
}

void GetPos(vec3_t origin, vec3_t angles)
{
	origin[0] = r_refdef.vieworg[0];
	origin[1] = r_refdef.vieworg[1];
	origin[2] = r_refdef.vieworg[2];

	angles[0] = r_refdef.viewangles[0];
	angles[1] = r_refdef.viewangles[1];
	angles[2] = r_refdef.viewangles[2];

	if (Cmd_Argc() == 2)
	{
		if (Q_atoi(Cmd_Argv(1)) == 2 && cls.state == ca_active)
		{
			origin[0] = cl.frames[cl.parsecountmod].playerstate[cl.playernum].origin[0];
			origin[1] = cl.frames[cl.parsecountmod].playerstate[cl.playernum].origin[1];
			origin[2] = cl.frames[cl.parsecountmod].playerstate[cl.playernum].origin[2];

			angles[0] = cl.frames[cl.parsecountmod].playerstate[cl.playernum].angles[0];
			angles[1] = cl.frames[cl.parsecountmod].playerstate[cl.playernum].angles[1];
			angles[2] = cl.frames[cl.parsecountmod].playerstate[cl.playernum].angles[2];
		}
	}
}

const char* CL_CleanFileName(const char* filename)
{
	if (filename && *filename && *filename == '!')
		return "customization";

	return filename;
}

void CL_ClearCaches()
{
	for (int i = 1; i < ARRAYSIZE(cl.event_precache) && cl.event_precache[i].pszScript; ++i)
	{
		Mem_Free(const_cast<char*>(cl.event_precache[i].pszScript));
		Mem_Free(const_cast<char*>(cl.event_precache[i].filename));

		Q_memset(&cl.event_precache[i], 0, sizeof(cl.event_precache[i]));
	}
}

void CL_ClearClientState()
{
	for (int i = 0; i < CL_UPDATE_BACKUP; ++i)
	{
		if (cl.frames[i].packet_entities.entities)
		{
			Mem_Free(cl.frames[i].packet_entities.entities);
		}

		cl.frames[i].packet_entities.entities = nullptr;
		cl.frames[i].packet_entities.num_entities = 0;
	}

	CL_ClearResourceLists();

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		COM_ClearCustomizationList(&cl.players[i].customdata, false);
	}

	CL_ClearCaches();

	Q_memset(&cl, 0, sizeof(cl));

	cl.resourcesneeded.pPrev = &cl.resourcesneeded;
	cl.resourcesneeded.pNext = &cl.resourcesneeded;
	cl.resourcesonhand.pPrev = &cl.resourcesonhand;
	cl.resourcesonhand.pNext = &cl.resourcesonhand;

	CL_CreateResourceList();
}

void CL_ClearState(bool bQuiet)
{
	if (!Host_IsServerActive())
		Host_ClearMemory(bQuiet);

	CL_ClearClientState();

	// TODO: implement - Solokiller
	// SZ_Clear( &cls.netchan.message );

	// clear other arrays
	Q_memset(cl_efrags, 0, sizeof(cl_efrags));
	Q_memset(cl_dlights, 0, sizeof(cl_dlights));
	Q_memset(cl_elights, 0, sizeof(cl_elights));
	Q_memset(cl_lightstyle, 0, sizeof(cl_lightstyle));

	CL_TempEntInit();

	//
	// allocate the efrags and chain together into a free list
	//
	cl.free_efrags = cl_efrags;

	int i;
	for (i = 0; i < MAX_EFRAGS - 1; ++i)
	{
		cl.free_efrags[i].entnext = &cl.free_efrags[i + 1];
	}

	cl.free_efrags[i].entnext = nullptr;
}

void CL_CreateResourceList()
{
	if (cls.state != ca_dedicated)
	{
		HPAK_FlushHostQueue();

		cl.num_resources = 0;

		char szFileName[MAX_PATH];
		snprintf(szFileName, ARRAYSIZE(szFileName), "tempdecal.wad");

		byte rgucMD5_hash[16];
		Q_memset(rgucMD5_hash, 0, sizeof(rgucMD5_hash));

		auto hFile = FS_Open(szFileName, "rb");

		if (FILESYSTEM_INVALID_HANDLE != hFile)
		{
			const auto uiSize = FS_Size(hFile);

			MD5_Hash_File(rgucMD5_hash, szFileName, false, false, nullptr);

			if (uiSize)
			{
				if (cl.num_resources > 1279)
					Sys_Error("Too many resources on client.");

				auto pResource = &cl.resourcelist[cl.num_resources];

				++cl.num_resources;

				Q_strncpy(pResource->szFileName, szFileName, ARRAYSIZE(pResource->szFileName));
				pResource->type = t_decal;
				pResource->nDownloadSize = uiSize;
				pResource->nIndex = 0;
				pResource->ucFlags |= RES_CUSTOM;

				Q_memcpy(pResource->rgucMD5_hash, rgucMD5_hash, sizeof(rgucMD5_hash));

				HPAK_AddLump(false, "custom.hpk", pResource, nullptr, hFile);
			}

			FS_Close(hFile);
		}
	}
}
