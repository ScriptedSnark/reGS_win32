#ifndef ENGINE_HOST_CMD_H
#define ENGINE_HOST_CMD_H

#include "GameUI/CareerDefs.h"


extern int gHostSpawnCount;
extern CareerStateType g_careerState;

extern bool g_iQuitCommandIssued;
extern bool g_bMajorMapChange;

typedef struct _UserMsg
{
	int iMsg;
	int iSize;
	char szName[16];
	struct _UserMsg* next;
	pfnUserMsgHook pfn;
} UserMsg;

void SV_GetPlayerHulls();

void Host_Map_f(void);
void Host_InitCommands();

#endif // ENGINE_HOST_CMD_H
