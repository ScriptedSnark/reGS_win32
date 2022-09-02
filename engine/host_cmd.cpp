#include "quakedef.h"
#include "client.h"
#include "dll_state.h"
#include "host_cmd.h"
#include "server.h"
#include "vgui_int.h"
#include "gl_screen.h"
#include "pr_cmds.h"
#include "pmove.h"
#include "sys_getmodes.h"
#include "gl_draw.h"

int gHostSpawnCount;

bool s_careerAudioPaused = false;

CareerStateType g_careerState = CAREER_NONE;

bool g_iQuitCommandIssued = false;

bool g_bMajorMapChange = false;

char szDirectory[260];

cvar_t motdfile = {"motdfile", "motd.txt", 0, 0.0f, NULL};

void SV_GetPlayerHulls()
{
	if (gEntityInterface.pfnGetHullBounds(0, (float*)player_mins, (float*)player_maxs) && gEntityInterface.pfnGetHullBounds(1, player_mins[1], player_maxs[1]) && gEntityInterface.pfnGetHullBounds(2, player_mins[2], player_maxs[2]))
	{
		gEntityInterface.pfnGetHullBounds(3, player_mins[3], player_maxs[3]);
	}
}

void Host_InitializeGameDLL()
{
	Cbuf_Execute();
	NET_Config((qboolean)svs.maxclients > 1);

	if (svs.dll_initialized)
	{
		Con_DPrintf("Sys_InitializeGameDLL called twice, skipping second call\n");
		return;
	}

	svs.dll_initialized = true;
	LoadEntityDLLs(host_parms.basedir);

	/* These funcs crash the game - ScriptedSnark */

	// gEntityInterface.pfnGameInit();
	// gEntityInterface.pfnPM_Init( &g_svmove );
	// gEntityInterface.pfnRegisterEncoders();

	/* - TODO: implement - ScriptedSnark
	SV_InitEncoders();
	SV_GetPlayerHulls();
	SV_CheckBlendingInterface();
	SV_CheckSaveGameCommentInterface();
	*/
	Cbuf_Execute();
}

void Host_Motd_f(void)
{
	int length;
	FileHandle_t pFile;
	char* pFileList;
	char* next;

	pFileList = motdfile.string;
	if (*pFileList == '/' || Q_strstr(pFileList, ":") || Q_strstr(pFileList, "..") || Q_strstr(pFileList, "\\"))
	{
		Con_Printf("Unable to open %s (contains illegal characters)\n", pFileList);
		return;
	}

	/* - TODO: fix this check
	if (Q_stricmp(COM_LastFileExtension(motdfile.string), "txt"))
	{
		Con_Printf("Invalid motdfile name %s (wrong file extension, must be .txt)\n", motdfile.string);
		return;
	}
	*/

	pFile = FS_Open(pFileList, "rb");
	if (!pFile)
	{
		Con_Printf("Unable to open %s\n", pFileList);
		return;
	}
	length = FS_Size(pFile);
	if (length > 0)
	{
		char* buf = (char*)Mem_Malloc(length + 1);
		if (buf)
		{
			FS_Read(buf, length, pFile);
			buf[length] = 0;
			char* now = buf;
			Con_Printf("motd:");
			next = strchr(now, '\n');
			while (next != NULL)
			{
				*next = 0;
				Con_Printf("%s\n", now);
				now = next + 1;
				next = strchr(now, '\n');
			}

			Con_Printf("%s\n", now);

			Mem_Free(buf);
		}
	}
	FS_Close(pFile);
}

void Host_ClearSaveDirectory()
{
	const char* i;	  // esi
	char szName[260]; // [esp+2Ch] [ebp-110h] BYREF

	Q_memset(szDirectory, 0, 260);
	Q_snprintf(szDirectory, sizeof(szDirectory), "SAVE/");
	Q_snprintf(szName, sizeof(szName), "%s", szDirectory);
	Q_strncat(szName, "*.HL?", sizeof(szName) - Q_strlen(szName) - 1);
	COM_FixSlashes(szName);

	if (Sys_FindFirstPathID(szName, "GAMECONFIG"))
	{
		Sys_FindClose();

		Q_memset(szDirectory, 0, 260);
		Q_snprintf(szDirectory, sizeof(szDirectory), "SAVE/");
		Q_snprintf(szName, sizeof(szName), "%s", szDirectory);

		COM_FixSlashes(szName);
		FS_CreateDirHierarchy(szName, "GAMECONFIG");
		Q_strncat(szName, "*.HL?", 259 - strlen(szName));

		for (i = Sys_FindFirstPathID(szName, "GAMECONFIG"); i; i = Sys_FindNext(0))
		{
			Q_memset(szDirectory, 0, 260);
			Q_snprintf(szDirectory, sizeof(szDirectory), "SAVE/");
			Q_snprintf(szName, sizeof(szName), "%s%s", szDirectory, i);
			FS_RemoveFile(szName, "GAMECONFIG");
		}
	}

	Sys_FindClose();
}

void Host_ClearGameState()
{
	S_StopAllSounds(true);
	Host_ClearSaveDirectory();

	if (gEntityInterface.pfnResetGlobalState)
		gEntityInterface.pfnResetGlobalState();
}

void Host_Map(qboolean bIsDemo, char* mapstring, char* mapName, qboolean loadGame)
{
	int i;
	UserMsg* pMsg;
	// Host_ShutdownServer(FALSE); - TODO: implement - ScriptedSnark
	key_dest = key_game;
	SCR_BeginLoadingPlaque(FALSE);
	if (!loadGame)
	{
		// Host_ClearGameState(); - TODO: implement - ScriptedSnark
		// SV_InactivateClients(); - TODO: implement - ScriptedSnark
		svs.serverflags = 0;
	}
	Q_strncpy(cls.mapstring, mapstring, sizeof(cls.mapstring) - 1);
	cls.mapstring[sizeof(cls.mapstring) - 1] = 0;
	if (SV_SpawnServer(bIsDemo, mapName, NULL))
	{
		ContinueLoadingProgressBar("Server", 7, 0.0);
		if (loadGame)
		{
			/* - TODO: implement - ScriptedSnark
			if (!LoadGamestate(mapName, 1))
				SV_LoadEntities();
			*/

			sv.paused = TRUE;
			sv.loadgame = TRUE;
			// SV_ActivateServer(0); - TODO: implement - ScriptedSnark
		}
		else
		{
			// SV_LoadEntities(); - TODO: implement - ScriptedSnark
			// SV_ActivateServer(1); - TODO: implement - ScriptedSnark
			if (!sv.active)
				return;

			if (cls.state != ca_dedicated)
			{
				Q_strcpy(cls.spawnparms, "");
				for (i = 0; i < Cmd_Argc(); i++)
					Q_strncat(cls.spawnparms, Cmd_Argv(i), sizeof(cls.spawnparms) - Q_strlen(cls.spawnparms) - 1);
			}
		}

		/* - TODO: implement - ScriptedSnark
		if (sv_gpNewUserMsgs)
		{
			pMsg = sv_gpUserMsgs;
			if (pMsg)
			{
				while (pMsg->next)
					pMsg = pMsg->next;
				pMsg->next = sv_gpNewUserMsgs;
			}
			else
				sv_gpUserMsgs = sv_gpNewUserMsgs;

			sv_gpNewUserMsgs = NULL;
		}
		*/

		if (cls.state)
			Cmd_ExecuteString("connect local", src_command);
	}
}

void Host_Map_f(void)
{
	char mapstring[MAX_QPATH];
	char name[MAX_QPATH];

	CareerStateType careerState = g_careerState;

	if (cmd_source != src_command)
	{
		g_careerState = CAREER_NONE;
		return;
	}

	if (Cmd_Argc() > 1 && Q_strlen(Cmd_Args()) > 54)
	{
		g_careerState = CAREER_NONE;
		Con_Printf("map change failed: command string is too long.\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		g_careerState = CAREER_NONE;
		Con_Printf("map <levelname> : changes server to specified map\n");
		return;
	}

	// CL_Disconnect(); - TODO: implement - ScriptedSnark

	if (careerState == CAREER_LOADING)
		g_careerState = CAREER_LOADING;

	if (COM_CheckParm("-steam") && PF_IsDedicatedServer())
		g_bMajorMapChange = true;

	FS_LogLevelLoadStarted("Map_Common");

	mapstring[0] = 0;

	for (int i = 0; i < Cmd_Argc(); i++)
	{
		strncat(mapstring, Cmd_Argv(i), 62 - Q_strlen(mapstring));
		strncat(mapstring, " ", 62 - Q_strlen(mapstring));
	}

	strcat(mapstring, "\n");
	Q_strncpy(name, Cmd_Argv(1), sizeof(name) - 1);
	name[sizeof(name) - 1] = 0;

	if (!svs.dll_initialized)
		Host_InitializeGameDLL();

	int iLen = Q_strlen(name);
	if (iLen > 4 && !Q_stricmp(&name[iLen - 4], ".bsp"))
		name[iLen - 4] = 0;

	FS_LogLevelLoadStarted(name);
	VGuiWrap2_LoadingStarted("level", name);

	// Force screen update
	SCR_UpdateScreen();
	SCR_UpdateScreen();

	if (!PF_IsMapValid_I(name))
	{
		Con_Printf("map change failed: '%s' not found on server.\n", name);

		if (COM_CheckParm("-steam"))
		{
			if (PF_IsDedicatedServer())
			{
				g_bMajorMapChange = false;
				Sys_Printf("\n");
			}
		}
		return;
	}

	StartLoadingProgressBar("Server", 24);
	SetLoadingProgressBarStatusText("#GameUI_StartingServer");
	ContinueLoadingProgressBar("Server", 1, 0.0);
	Cvar_Set("HostMap", name);

	Host_Map(false, mapstring, name, false);

	if (COM_CheckParm("-steam") && PF_IsDedicatedServer())
	{
		g_bMajorMapChange = false;
		Sys_Printf("\n");
	}

	ContinueLoadingProgressBar("Server", 11, 0.0);
	// NotifyDedicatedServerUI("UpdateMap"); - TODO: implement - ScriptedSnark

	if (g_careerState == CAREER_LOADING)
	{
		g_careerState = CAREER_PLAYING;
		//SetCareerAudioState(true);
	}
	/*
	else
		SetCareerAudioState(false);
	*/
}


void Host_Maps_f()
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("Usage:  maps <substring>\nmaps * for full listing\n");
	}

	const char* pszSubString = (char*)Cmd_Argv(1);

	if (pszSubString && *pszSubString)
	{
		if (*pszSubString == '*')
			pszSubString = nullptr;

		COM_ListMaps(pszSubString);
	}
}

void Host_SetVideoMode_f()
{
	int width = Q_atoi(Cmd_Argv(1));
	int height = Q_atoi(Cmd_Argv(2));
	int bpp = 16;

	if (cls.state != ca_dedicated && (Cmd_Argc() == 3 || Cmd_Argc() == 4))
	{
		if (Cmd_Argc() == 4)
			bpp = Q_atoi(Cmd_Argv(3));

		VideoMode_SetVideoMode(width, height, bpp);
	}
}

void Host_SetRenderer_f() // TODO: improve checks - ScriptedSnark
{
	int renderer = 0;
	int windowed = FALSE;

	if (cls.state != ca_dedicated && (Cmd_Argc() == 2 || Cmd_Argc() == 3))
	{
		if (Q_strcmp(Cmd_Argv(1), "software"))
		{
			renderer = 1;

			if (Q_strcmp(Cmd_Argv(1), "gl"))
				renderer = 2 - (Q_strcmp(Cmd_Argv(1), "d3d") != 0);
		}

		if (Cmd_Argc() == 3)
			windowed = Q_strcmp(Cmd_Argv(2), "windowed") == 0;

		VideoMode_SwitchMode(renderer, windowed);
	}
}

void Host_Quit_f()
{
	if (Cmd_Argc() == 1)
	{
		giActive = DLL_CLOSE;
		g_iQuitCommandIssued = true;
		// TODO: implement - Solokiller
		/*
		if( cls.state != ca_dedicated )
			CL_Disconnect();
		Host_ShutdownServer( 0 );
		*/
		Sys_Quit();
	}
	else
	{
		giActive = DLL_PAUSED;
		giStateInfo = 4;
	}
}

void Host_Quit_Restart_f()
{
	giActive = DLL_STATE_RESTART;
	giStateInfo = 4;

	/* TODO: implement after reversing game world/server/client  - THIS CODE IS DECOMPILED, NEEDED TO CORRECT - ScriptedSnark
	if ( sv.active
	|| cls.state == ca_active
	&& cls.trueaddress[0]
	&& g_pPostRestartCmdLineArgs
	&& (strcat(g_pPostRestartCmdLineArgs, " +connect "), strcat(g_pPostRestartCmdLineArgs, cls.servername), sv.active) )
	{
		if ( svs.maxclients == 1 && cls.state == ca_active )
		{
			if ( g_pPostRestartCmdLineArgs )
			{
				Cbuf_AddText("save quick\n");
				Cbuf_Execute();
				strcat(g_pPostRestartCmdLineArgs, " +load quick");
			}
	}
	*/
}

void Host_InitCommands()
{
	if (!g_bIsDedicatedServer)
	{
		//Cmd_AddCommand("cd", CD_Command_f);
		//Cmd_AddCommand("mp3", MP3_Command_f);
		Cmd_AddCommand("_restart", Host_Quit_Restart_f);
		Cmd_AddCommand("_setrenderer", Host_SetRenderer_f);
		Cmd_AddCommand("_sethdmodels", Host_SetHDModels_f);
		Cmd_AddCommand("_setaddons_folder", Host_SetAddonsFolder_f);
		Cmd_AddCommand("_set_vid_level", Host_SetVideoLevel_f);
		Cmd_AddCommand("_setvideomode", Host_SetVideoMode_f);
		// Cmd_AddCommand("_careeraudio", CareerAudio_Command_f); TODO: Implement
	}

	// TODO: implement - Solokiller
	Cmd_AddCommand("quit", Host_Quit_f);
	Cmd_AddCommand("exit", Host_Quit_f);
	Cmd_AddCommand("map", Host_Map_f);
	Cmd_AddCommand("maps", Host_Maps_f);
	Cmd_AddCommand("motd", Host_Motd_f);
	// TODO: implement - Solokiller
}
