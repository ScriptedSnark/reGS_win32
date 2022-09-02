#include "quakedef.h"
#include "client.h"
#include "cl_parse.h"
#include "host_cmd.h"

int CL_UPDATE_BACKUP = 1 << 6;
int CL_UPDATE_MASK = CL_UPDATE_BACKUP - 1;
UserMsg* gClientUserMsgs;

void CL_ShutDownUsrMessages()
{
	UserMsg* p = gClientUserMsgs; // eax

	if (gClientUserMsgs)
	{
		while (1)
		{
			Mem_Free(p->next);

			if (!p->next)
				break;

			p = p->next;
		}

		gClientUserMsgs = NULL;
	}
}

pfnUserMsgHook HookServerMsg(const char* pszName, pfnUserMsgHook pfn)
{
	// TODO: implement - Solokiller
	return nullptr;
}

int DispatchDirectUserMsg(const char* pszName, int iSize, void* pBuf)
{
	for (UserMsg* i = gClientUserMsgs; i; i = i->next)
	{
		if (!Q_stricmp(pszName, i->szName))
		{
			if (i->iSize == -1)
				i->iSize = iSize;

			if (i->pfn)
				i->pfn(i->szName, iSize, pBuf);
			else
				Con_DPrintf("UserMsg: No pfn %s %d\n", i->szName, i->iMsg);

			return 1;
		}
	}
	return 0;
}

void CL_RemoveFromResourceList(resource_t* pResource)
{
	if (!pResource->pPrev || !pResource->pNext)
		Sys_Error("Mislinked resource in CL_RemoveFromResourceList");

	if (pResource == pResource->pNext || pResource->pPrev == pResource)
		Sys_Error("Attempt to free last entry in list.");

	pResource->pPrev->pNext = pResource->pNext;
	pResource->pNext->pPrev = pResource->pPrev;
	pResource->pPrev = nullptr;
	pResource->pNext = nullptr;
}

void CL_ClearResourceLists()
{
	cl.downloadUrl[0] = '\0';

	for (resource_t *pResource = cl.resourcesneeded.pNext, *pNext;
		 pResource != &cl.resourcesneeded;
		 pResource = pNext)
	{
		pNext = pResource->pNext;

		CL_RemoveFromResourceList(pResource);
		Mem_Free(pResource);
	}

	cl.resourcesneeded.pNext = &cl.resourcesneeded;
	cl.resourcesneeded.pPrev = &cl.resourcesneeded;

	for (resource_t *pResource = cl.resourcesonhand.pNext, *pNext;
		 pResource != &cl.resourcesonhand;
		 pResource = pNext)
	{
		pNext = pResource->pNext;

		CL_RemoveFromResourceList(pResource);
		Mem_Free(pResource);
	}

	cl.resourcesonhand.pNext = &cl.resourcesonhand;
	cl.resourcesonhand.pPrev = &cl.resourcesonhand;
}

qboolean CL_CheckGameDirectory(char* gamedir)
{
	char szGD[260];	 // [esp+1Ch] [ebp-110h] BYREF

	if (gamedir && *gamedir)
	{
		COM_FileBase(com_gamedir, szGD);

		if (Q_stricmp(szGD, gamedir))
		{
			Con_Printf("Server is running game %s.  Restart in that game to connect.\n", gamedir);
			//CL_Disconnect_f();
			return false;
		}

		return true;
	}
	else
	{
		Con_Printf("Server didn't specify a gamedir, assuming no change\n");
		return true;
	}

	return false;
}