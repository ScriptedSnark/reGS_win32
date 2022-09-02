#include "quakedef.h"

#include "client.h"
#include "modinfo.h"
#include "pr_edict.h"
#include "sv_main.h"
#include "sv_phys.h"
#include "server.h"
#include "host.h"
#include "vgui_int.h"
#include "cl_main.h"
#include "host_cmd.h"
#include "gl_screen.h"
#include "gl_model.h"
#include <cmodel.h>
#include <pmove.h>

edict_t** g_moved_edict;
vec3_t* g_moved_from;
char* gNullString = "";
char* pr_strings;
delta_info_t* g_sv_delta;
redirect_t sv_redirected;
netadr_t sv_redirectto;

char outputbuf[MAX_ROUTEABLE_PACKET];

server_static_t svs;
server_t sv;

// TODO: implement functions and add them - Solokiller
playermove_t g_svmove;
globalvars_t gGlobalVariables = {};

bool allow_cheats = false;
qboolean g_bOutOfDateRestart;

cvar_t sv_allow_upload = {"sv_allowupload", "1", FCVAR_SERVER};
cvar_t mapcyclefile = {"mapcyclefile", "mapcycle.txt"};
cvar_t servercfgfile = {"servercfgfile", "server.cfg"};
cvar_t max_queries_sec = {"max_queries_sec", "3.0", FCVAR_PROTECTED | FCVAR_SERVER};
cvar_t max_queries_sec_global = {"max_queries_sec_global", "30", FCVAR_PROTECTED | FCVAR_SERVER};
cvar_t max_queries_window = {"max_queries_window", "60", FCVAR_PROTECTED | FCVAR_SERVER};

int SV_UPDATE_BACKUP = 1 << 3;
int SV_UPDATE_MASK = SV_UPDATE_BACKUP - 1;

char localmodels[MAX_MODELS][5];

struct GameToAppIDMapItem_t
{
	AppId_t iAppID;
	const char* pGameDir;
};

const AppId_t HALF_LIFE_APPID = 70;

const GameToAppIDMapItem_t g_GameToAppIDMap[] =
	{
		{10, "cstrike"},
		{20, "tfc"},
		{30, "dod"},
		{40, "dmc"},
		{50, "gearbox"},
		{60, "ricochet"},
		{HALF_LIFE_APPID, "valve"},
		{80, "czero"},
		{100, "czeror"},
		{130, "bshift"},
		{150, "cstrike_beta"}};

modinfo_t gmodinfo = {};

delta_t* SV_LookupDelta(char* name)
{
	delta_info_t* p = g_sv_delta;

	while (p)
	{
		if (!Q_stricmp(name, p->name))
		{
			return p->delta;
		}
		p = p->next;
	}

	Sys_Error("%s: Couldn't find delta for %s\n", __func__, name);
}

AppId_t GetGameAppID()
{
	char gd[FILENAME_MAX];
	char arg[FILENAME_MAX];

	COM_ParseDirectoryFromCmd("-game", arg, "valve");
	COM_FileBase(arg, gd);

	for (const auto& data : g_GameToAppIDMap)
	{
		if (!stricmp(data.pGameDir, gd))
		{
			return data.iAppID;
		}
	}

	return HALF_LIFE_APPID;
}

bool IsGameSubscribed(const char* game)
{
	AppId_t appId = HALF_LIFE_APPID;

	for (const auto& data : g_GameToAppIDMap)
	{
		if (!stricmp(data.pGameDir, game))
		{
			appId = data.iAppID;
		}
	}

	return ISteamApps_BIsSubscribedApp(appId);
}

bool BIsValveGame()
{
	for (const auto& data : g_GameToAppIDMap)
	{
		if (!stricmp(data.pGameDir, com_gamedir))
		{
			return false;
		}
	}

	return true;
}

static bool g_bCS_CZ_Flags_Initialized = false;

bool g_bIsCStrike = false;
bool g_bIsCZero = false;
bool g_bIsCZeroRitual = false;
bool g_bIsTerrorStrike = false;
bool g_bIsTFC = false;

void SetCStrikeFlags()
{
	if (!g_bCS_CZ_Flags_Initialized)
	{
		if (!stricmp(com_gamedir, "cstrike") ||
			!stricmp(com_gamedir, "cstrike_beta"))
		{
			g_bIsCStrike = true;
		}
		else if (!stricmp(com_gamedir, "czero"))
		{
			g_bIsCZero = true;
		}
		else if (!stricmp(com_gamedir, "czeror"))
		{
			g_bIsCZeroRitual = true;
		}
		else if (!stricmp(com_gamedir, "terror"))
		{
			g_bIsTerrorStrike = true;
		}
		else if (!stricmp(com_gamedir, "tfc"))
		{
			g_bIsTFC = true;
		}

		g_bCS_CZ_Flags_Initialized = true;
	}
}

void SV_DeallocateDynamicData()
{
	if (g_moved_edict)
		Mem_Free(g_moved_edict);
	if (g_moved_from)
		Mem_Free(g_moved_from);

	g_moved_edict = NULL;
	g_moved_from = NULL;
}

void SV_AllocClientFrames()
{
	for (int i = 0; i < svs.maxclientslimit; ++i)
	{
		if (svs.clients[i].frames)
			Con_DPrintf("Allocating over frame pointer?\n");

		svs.clients[i].frames = reinterpret_cast<client_frame_t*>(Mem_ZeroMalloc(sizeof(client_frame_t) * SV_UPDATE_BACKUP));
	}
}

void SV_ClearFrames(client_frame_t** frames)
{
	if (*frames)
	{
		auto pFrame = *frames;

		for (int i = 0; i < SV_UPDATE_BACKUP; ++i, ++pFrame)
		{
			if (pFrame->entities.entities)
			{
				Mem_Free(pFrame->entities.entities);
			}

			pFrame->entities.entities = nullptr;
			pFrame->entities.num_entities = 0;

			pFrame->senttime = 0;
			pFrame->ping_time = -1;
		}

		Mem_Free(*frames);
		*frames = nullptr;
	}
}

void SV_ResetModInfo()
{
	Q_memset(&gmodinfo, 0, sizeof(gmodinfo));

	gmodinfo.version = 1;
	gmodinfo.svonly = true;
	gmodinfo.num_edicts = MAX_EDICTS;

	char szDllListFile[FILENAME_MAX];
	snprintf(szDllListFile, sizeof(szDllListFile), "%s", "liblist.gam");

	FileHandle_t hLibListFile = FS_Open(szDllListFile, "rb");

	if (hLibListFile != FILESYSTEM_INVALID_HANDLE)
	{
		char szKey[64];
		char szValue[256];

		const int iSize = FS_Size(hLibListFile);

		if (iSize > (512 * 512) || !iSize)
			Sys_Error("Game listing file size is bogus [%s: size %i]", "liblist.gam", iSize);

		byte* pFileData = reinterpret_cast<byte*>(Mem_Malloc(iSize + 1));

		if (!pFileData)
			Sys_Error("Could not allocate space for game listing file of %i bytes", iSize + 1);

		const int iRead = FS_Read(pFileData, iSize, hLibListFile);

		if (iRead != iSize)
			Sys_Error("Error reading in game listing file, expected %i bytes, read %i", iSize, iRead);

		pFileData[iSize] = '\0';

		char* pBuffer = (char*)pFileData;

		com_ignorecolons = true;

		while (1)
		{
			pBuffer = COM_Parse(pBuffer);

			if (Q_strlen(com_token) <= 0)
				break;

			Q_strncpy(szKey, com_token, sizeof(szKey) - 1);
			szKey[sizeof(szKey) - 1] = '\0';

			pBuffer = COM_Parse(pBuffer);

			Q_strncpy(szValue, com_token, sizeof(szValue) - 1);
			szValue[sizeof(szValue) - 1] = '\0';

			if (Q_stricmp(szKey, "gamedll_linux"))
				DLL_SetModKey(&gmodinfo, szKey, szValue);
		}

		com_ignorecolons = false;
		Mem_Free(pFileData);
		FS_Close(hLibListFile);
	}
}

void SV_FlushRedirect(void)
{
	unsigned char* data;
	sizebuf_t buf;

	if (sv_redirected == RD_PACKET)
	{
		int allocLen = Q_strlen(outputbuf) + 22;
		data = (unsigned char*)_malloca(allocLen);

		buf.buffername = "Redirected Text";
		buf.data = data;
		buf.maxsize = Q_strlen(outputbuf) + 7;
		buf.cursize = 0;
		buf.flags = 1;

		MSG_WriteLong(&buf, -1);
		MSG_WriteByte(&buf, A2A_PRINT);
		MSG_WriteString(&buf, outputbuf);
		MSG_WriteByte(&buf, 0);
		NET_SendPacket(NS_SERVER, buf.cursize, buf.data, sv_redirectto);
		outputbuf[0] = 0;
	}
	else
	{
		if (sv_redirected == RD_CLIENT)
		{
			MSG_WriteByte(&host_client->netchan.message, svc_print);
			MSG_WriteString(&host_client->netchan.message, outputbuf);
		}
		outputbuf[0] = 0;
	}
}

int SV_SpawnServer(qboolean bIsDemo, char* server, char* startspot)
{
	client_t* cl;
	edict_t* ent;
	int i;
	char* pszhost;
	char oldname[64];

	if (sv.active)
	{
		cl = svs.clients;
		for (i = 0; i < svs.maxclients; i++, cl++)
		{
			if (cl->active || cl->spawned || cl->connected)
			{
				ent = cl->edict;
				if (ent == NULL || ent->free)
					continue;

				if (ent->pvPrivateData)
					gEntityInterface.pfnClientDisconnect(ent);
				else
					Con_Printf("Skipping reconnect on %s, no pvPrivateData\n", cl->name);
			}
		}
	}
	if (g_bOutOfDateRestart)
	{
		g_bOutOfDateRestart = FALSE;
		Cmd_ExecuteString("quit\n", src_command);
	}

	Log_Open();
	Log_Printf("Loading map \"%s\"\n", server);
	Log_PrintServerVars();
	NET_Config((qboolean)(svs.maxclients > 1));

	pszhost = (char*)Cvar_VariableString("hostname");
	if (pszhost && *pszhost == '\0')
	{
		if (gEntityInterface.pfnGetGameDescription != NULL)
			Cvar_Set("hostname", gEntityInterface.pfnGetGameDescription());
		else
			Cvar_Set("hostname", "Half-Life");
	}

	scr_centertime_off = 0.0f;
	if (startspot)
		Con_DPrintf("Spawn Server %s: [%s]\n", server, startspot);
	else
		Con_DPrintf("Spawn Server %s\n", server);

	g_LastScreenUpdateTime = 0.0f;
	svs.spawncount = ++gHostSpawnCount;

	/* - TODO: implement - ScriptedSnark
	if (coop.value != 0.0f)
		Cvar_SetValue("deathmatch", 0.0f);
	*/

	/*
	current_skill = (int)(skill.value + 0.5f);
	if (current_skill < 0)
		current_skill = 0;
	else if (current_skill > 3)
		current_skill = 3;
	*/
	// Cvar_SetValue("skill", current_skill);
	ContinueLoadingProgressBar("Server", 2, 0.0f);

	// HPAK_CheckSize("custom");
	oldname[0] = 0;
	Q_strncpy(oldname, sv.name, sizeof(oldname) - 1);
	oldname[sizeof(oldname) - 1] = 0;
	Host_ClearMemory(FALSE);

	cl = svs.clients;
	for (i = 0; i < svs.maxclientslimit; i++, cl++)
		SV_ClearFrames(&cl->frames);

	// SV_UPDATE_BACKUP = (svs.maxclients == 1) ? SINGLEPLAYER_BACKUP : MULTIPLAYER_BACKUP;
	// SV_UPDATE_MASK = (SV_UPDATE_BACKUP - 1);

	SV_AllocClientFrames();
	Q_memset(&sv, 0, sizeof(server_t));

	Q_strncpy(sv.oldname, oldname, sizeof(oldname) - 1);
	sv.oldname[sizeof(oldname) - 1] = 0;
	Q_strncpy(sv.name, server, sizeof(sv.name) - 1);
	sv.name[sizeof(sv.name) - 1] = 0;

	if (startspot)
	{
		Q_strncpy(sv.startspot, startspot, sizeof(sv.startspot) - 1);
		sv.startspot[sizeof(sv.startspot) - 1] = 0;
	}
	else
		sv.startspot[0] = 0;

	pr_strings = gNullString;
	gGlobalVariables.pStringBase = gNullString;

	if (svs.maxclients == 1)
		Cvar_SetValue("sv_clienttrace", 1.0);

	sv.max_edicts = COM_EntsForPlayerSlots(svs.maxclients);

	SV_DeallocateDynamicData();
	// SV_ReallocateDynamicData();

	gGlobalVariables.maxEntities = sv.max_edicts;

	sv.edicts = (edict_t*)Hunk_AllocName(sizeof(edict_t) * sv.max_edicts, "edicts");
	sv.baselines = (entity_state_s*)Hunk_AllocName(sizeof(entity_state_t) * sv.max_edicts, "baselines");

	sv.datagram.buffername = "Server Datagram";
	sv.datagram.data = sv.datagram_buf;
	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.cursize = 0;

	sv.reliable_datagram.buffername = "Server Reliable Datagram";
	sv.reliable_datagram.data = sv.reliable_datagram_buf;
	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.cursize = 0;

	sv.spectator.buffername = "Server Spectator Buffer";
	sv.spectator.data = sv.spectator_buf;
	sv.spectator.maxsize = sizeof(sv.spectator_buf);

	sv.multicast.buffername = "Server Multicast Buffer";
	sv.multicast.data = sv.multicast_buf;
	sv.multicast.maxsize = sizeof(sv.multicast_buf);

	sv.signon.buffername = "Server Signon Buffer";
	sv.signon.data = sv.signon_data;
	sv.signon.maxsize = sizeof(sv.signon_data);
	sv.signon.cursize = 0;

	sv.num_edicts = svs.maxclients + 1;

	cl = svs.clients;
	for (i = 1; i < svs.maxclients; i++, cl++)
		cl->edict = &sv.edicts[i];

	gGlobalVariables.maxClients = svs.maxclients;
	sv.time = 1.0f;
	sv.state = ss_loading;
	sv.paused = FALSE;
	gGlobalVariables.time = 1.0f;

	// R_ForceCVars((qboolean)(svs.maxclients > 1));
	ContinueLoadingProgressBar("Server", 3, 0.0f);

	Q_snprintf(sv.modelname, sizeof(sv.modelname), "maps/%s.bsp", server);
	sv.worldmodel = Mod_ForName(sv.modelname, FALSE, FALSE);

	if (!sv.worldmodel)
	{
		Con_Printf("Couldn't spawn server %s\n", sv.modelname);
		sv.active = FALSE;
		return 0;
	}

	// Sequence_OnLevelLoad(server);
	ContinueLoadingProgressBar("Server", 4, 0.0);
	if (gmodinfo.clientcrccheck)
	{
		char szDllName[64];
		Q_snprintf(szDllName, sizeof(szDllName), "cl_dlls//client.dll");
		COM_FixSlashes(szDllName);
		if (!MD5_Hash_File(sv.clientdllmd5, szDllName, FALSE, FALSE, NULL))
		{
			Con_Printf("Couldn't CRC client side dll:  %s\n", szDllName);
			sv.active = FALSE;
			return 0;
		}
	}
	ContinueLoadingProgressBar("Server", 6, 0.0);

	if (svs.maxclients <= 1)
		sv.worldmapCRC = 0;
	else
	{
		CRC32_Init(&sv.worldmapCRC);
		if (!CRC_MapFile(&sv.worldmapCRC, sv.modelname))
		{
			Con_Printf("Couldn't CRC server map:  %s\n", sv.modelname);
			sv.active = FALSE;
			return 0;
		}

		CM_CalcPAS(sv.worldmodel);
	}

	sv.models[1] = sv.worldmodel;
	// SV_ClearWorld();
	sv.model_precache_flags[1] |= RES_FATALIFMISSING;
	sv.model_precache[1] = sv.modelname;


	sv.sound_precache[0] = pr_strings;
	sv.model_precache[0] = pr_strings;
	sv.generic_precache[0] = pr_strings;

	for (i = 1; i < sv.worldmodel->numsubmodels; i++)
	{
		sv.model_precache[i + 1] = localmodels[i];
		// sv.models[i + 1] = Mod_ForName(localmodels[i], FALSE, FALSE);
		sv.model_precache_flags[i + 1] |= RES_FATALIFMISSING;
	}

	Q_memset(&sv.edicts->v, 0, sizeof(entvars_t));

	sv.edicts->free = FALSE;
	sv.edicts->v.modelindex = 1;
	sv.edicts->v.model = (size_t)sv.worldmodel - (size_t)pr_strings;
	sv.edicts->v.solid = SOLID_BSP;
	sv.edicts->v.movetype = MOVETYPE_PUSH;

	/*
	if (coop.value == 0.0f)
		gGlobalVariables.deathmatch_ = deathmatch.value;
	else
		gGlobalVariables.coop_ = coop.value;
	*/
	gGlobalVariables.serverflags = svs.serverflags;
	gGlobalVariables.mapname = (size_t)sv.name - (size_t)pr_strings;
	gGlobalVariables.startspot = (size_t)sv.startspot - (size_t)pr_strings;
	// allow_cheats = sv_cheats.value;
	// SV_SetMoveVars();

	return 1;
}

void SV_ServerShutdown()
{
	// Steam_NotifyOfLevelChange(); - TODO: implement - ScriptedSnark
	gGlobalVariables.time = sv.time;
	if (svs.dll_initialized)
	{
		if (sv.active)
			gEntityInterface.pfnServerDeactivate();
	}
}

void SV_InitEncoders(void)
{
	delta_t* pdesc;
	delta_info_t* p;
	for (p = g_sv_delta; p != NULL; p = p->next)
	{
		pdesc = p->delta;
		if (Q_strlen(pdesc->conditionalencodename) > 0)
			pdesc->conditionalencode = DELTA_LookupEncoder(pdesc->conditionalencodename);
	}
}

void SV_Init()
{
	// TODO: implement - Solokiller
	Cvar_RegisterVariable(&sv_gravity);
	// TODO: implement - Solokiller
	Cvar_RegisterVariable(&sv_allow_upload);
	// TODO: implement - Solokiller
	Cvar_RegisterVariable(&motdfile);
	Cvar_RegisterVariable(&mapcyclefile);
	// TODO: implement - Solokiller
	Cvar_RegisterVariable(&servercfgfile);
	// TODO: implement - Solokiller
	Cvar_RegisterVariable(&max_queries_sec);
	Cvar_RegisterVariable(&max_queries_sec_global);
	Cvar_RegisterVariable(&max_queries_window);
	// TODO: implement - Solokiller

	PM_Init(&g_svmove);
}

void SV_Shutdown()
{
	delta_info_t* p = g_sv_delta;
	while (p)
	{
		delta_info_t* n = p->next;
		if (p->delta)
			DELTA_FreeDescription(&p->delta);

		Mem_Free(p->name);
		Mem_Free(p->loadfile);
		Mem_Free(p);
		p = n;
	}

	g_sv_delta = NULL;
}

void SV_SetMaxclients()
{
	for (int i = 0; i < svs.maxclientslimit; ++i)
	{
		SV_ClearFrames(&svs.clients[i].frames);
	}

	svs.maxclients = 1;

	const int iCmdMaxPlayers = COM_CheckParm("-maxplayers");

	if (iCmdMaxPlayers)
	{
		svs.maxclients = Q_atoi(com_argv[iCmdMaxPlayers + 1]);
	}

	cls.state = g_bIsDedicatedServer ? ca_dedicated : ca_disconnected;

	if (svs.maxclients <= 0)
	{
		svs.maxclients = MP_MIN_CLIENTS;
	}
	else if (svs.maxclients > MAX_CLIENTS)
	{
		svs.maxclients = MAX_CLIENTS;
	}

	svs.maxclientslimit = MAX_CLIENTS;

	// If we're a listen server and we're low on memory, reduce maximum player limit.
	if (!g_bIsDedicatedServer && host_parms.memsize < LISTENSERVER_SAFE_MINIMUM_MEMORY)
		svs.maxclientslimit = 4;

	// Increase the number of updates available for multiplayer.
	SV_UPDATE_BACKUP = 8;

	if (svs.maxclients != 1)
		SV_UPDATE_BACKUP = 64;

	SV_UPDATE_MASK = SV_UPDATE_BACKUP - 1;

	svs.clients = reinterpret_cast<client_t*>(Hunk_AllocName(sizeof(client_t) * svs.maxclientslimit, "clients"));

	for (int i = 0; i < svs.maxclientslimit; ++i)
	{
		auto& client = svs.clients[i];

		Q_memset(&client, 0, sizeof(client));
		client.resourcesneeded.pPrev = &client.resourcesneeded;
		client.resourcesneeded.pNext = &client.resourcesneeded;
		client.resourcesonhand.pPrev = &client.resourcesonhand;
		client.resourcesonhand.pNext = &client.resourcesonhand;
	}

	Cvar_SetValue("deathmatch", svs.maxclients > 1 ? 1 : 0);

	SV_AllocClientFrames();

	int maxclients = svs.maxclientslimit;

	if (maxclients > svs.maxclients)
		maxclients = svs.maxclients;

	svs.maxclients = maxclients;
}

void SV_CountPlayers(int* clients)
{
	*clients = 0;

	for (int i = 0; i < svs.maxclients; ++i)
	{
		auto& client = svs.clients[i];

		if (client.active || client.spawned || client.connected)
		{
			++*clients;
		}
	}
}

void SV_KickPlayer(int nPlayerSlot, int nReason)
{
	// TODO: implement - Solokiller
}

void SV_ClearEntities()
{
	for (int i = 0; i < sv.num_edicts; ++i)
	{
		if (!sv.edicts[i].free)
		{
			// TODO: need to increment the serial number so EHANDLE works properly - Solokiller
			ReleaseEntityDLLFields(&sv.edicts[i]);
		}
	}
}

void SV_ClearCaches()
{
	for (int i = 1; i < ARRAYSIZE(sv.event_precache) && sv.event_precache[i].filename; ++i)
	{
		auto& event = sv.event_precache[i];

		event.filename = nullptr;

		if (event.pszScript)
			Mem_Free(const_cast<char*>(event.pszScript));

		event.pszScript = nullptr;
	}
}

void SV_PropagateCustomizations()
{
	// TODO: implement - Solokiller
}

void SV_FailDownload(char* filename)
{
	if (filename && *filename)
	{
		MSG_WriteByte(&host_client->netchan.message, 49);
		MSG_WriteString(&host_client->netchan.message, filename);
	}
}

qboolean IsSafeFileToDownload(const char* filename)
{
	char lwrfilename[4096]; // [esp+10h] [ebp-100Ch] BYREF

	if (filename)
	{
		Q_strncpy(lwrfilename, filename, sizeof(lwrfilename));
		/*Q_*/ strlwr(lwrfilename);
		if (!Q_strstr(lwrfilename, ":") && !Q_strstr(lwrfilename, "..") && !Q_strstr(lwrfilename, "~"))
		{
			if (lwrfilename[0] == 92 || lwrfilename[0] == 47)
			{
				return FALSE;
			}
			else
			{
				if (strchr(lwrfilename, 46) == strrchr(lwrfilename, 46) // TODO: improve checks - ScriptedSnark
					&& !Q_stristr(lwrfilename, ".cfg")					// "autoexec." in pseudocode
					&& !Q_stristr(lwrfilename, ".rc") && !Q_stristr(lwrfilename, ".lst") && !Q_stristr(lwrfilename, ".exe") && !Q_stristr(lwrfilename, ".vbs") && !Q_stristr(lwrfilename, ".com") && !Q_stristr(lwrfilename, ".bat") && !Q_stristr(lwrfilename, ".dll") && !Q_stristr(lwrfilename, ".ini") && !Q_stristr(lwrfilename, ".") && !Q_stristr(lwrfilename, "halflife.wad") && !Q_stristr(lwrfilename, "pak0.pak") && !Q_stristr(lwrfilename, "xeno.wad") && !Q_stristr(lwrfilename, ".so") && !Q_stristr(lwrfilename, ".dylib") && !Q_stristr(lwrfilename, ".sys") && !Q_stristr(lwrfilename, ".asi") && !Q_stristr(lwrfilename, ".mix"))
				{
					return Q_stristr(lwrfilename, ".flt") == FALSE;
				}
			}
		}
	}

	return FALSE;
}
