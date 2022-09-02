#ifndef ENGINE_CL_PARSE_H
#define ENGINE_CL_PARSE_H

#include "cdll_int.h"

extern int CL_UPDATE_BACKUP;
extern int CL_UPDATE_MASK;

void CL_ShutDownUsrMessages();

pfnUserMsgHook HookServerMsg(const char* pszName, pfnUserMsgHook pfn);

int DispatchDirectUserMsg(const char* pszName, int iSize, void* pBuf);

void CL_RemoveFromResourceList(resource_t* pResource);

void CL_ClearResourceLists();

#endif // ENGINE_CL_PARSE_H
