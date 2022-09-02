#ifndef ENGINE_SV_MAIN_H
#define ENGINE_SV_MAIN_H

#include "steam/steam_api.h"
#include "delta.h"

struct client_frame_t;

extern delta_info_t* g_sv_delta;

extern bool allow_cheats;

extern cvar_t sv_allow_upload;

extern cvar_t mapcyclefile;
extern cvar_t servercfgfile;

extern cvar_t max_queries_sec;
extern cvar_t max_queries_sec_global;
extern cvar_t max_queries_window;

delta_t* SV_LookupDelta(char* name);
AppId_t GetGameAppID();

bool IsGameSubscribed(const char* game);
bool BIsValveGame();

extern bool g_bIsCStrike;
extern bool g_bIsCZero;
extern bool g_bIsCZeroRitual;
extern bool g_bIsTerrorStrike;
extern bool g_bIsTFC;

void SetCStrikeFlags();

void SV_DeallocateDynamicData();

void SV_AllocClientFrames();

void SV_ClearFrames(client_frame_t** frames);

int SV_SpawnServer(qboolean bIsDemo, char* server, char* startspot);

void SV_ServerShutdown();

void SV_Init();

void SV_Shutdown();

void SV_SetMaxclients();

void SV_CountPlayers(int* clients);

void SV_KickPlayer(int nPlayerSlot, int nReason);

void SV_ClearEntities();

void SV_ClearCaches();

void SV_PropagateCustomizations();

qboolean IsSafeFileToDownload(const char* filename);

#endif // ENGINE_SV_MAIN_H
