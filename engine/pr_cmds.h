#ifndef ENGINE_PR_CMDS_H
#define ENGINE_PR_CMDS_H

#include "tier0/platform.h"

qboolean PF_IsDedicatedServer();
int PF_IsMapValid_I(char* mapname);

void SeedRandomNumberGenerator();

float RandomFloat(float flLow, float flHigh);
int32 RandomLong(int32 lLow, int32 lHigh);

int hudCheckParm(char* parm, char** ppnext);

#endif // ENGINE_PR_CMDS_H
