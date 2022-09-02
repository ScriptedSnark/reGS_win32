#include "quakedef.h"
#include "cl_parsefn.h"
#include "module.h"
#include "sound.h"
#include "client.h"

event_hook_t* g_pEventHooks;

void CL_InitEventSystem()
{
	g_pEventHooks = 0;
}

void CL_HookEvent(char* name, void (*pfnEvent)(event_args_t*)) // here my reverse-engineering skill is bad
{
	event_hook_t* evhook;	  // ebx
	event_hook_t* eventhooks; // eax

	g_engdstAddrs.pfnHookEvent(&name, &pfnEvent);

	if (!pfnEvent)
	{
		Con_Printf("CL_HookEvent:  Must provide an event hook callback\n", &pfnEvent);
		return;
	}

	if (!g_pEventHooks)
	{
		Con_Printf("CL_HookEvent:  Must provide a valid event name\n", &pfnEvent);
		return;
	}

	if (name && *name)
	{
		while (!g_pEventHooks->name || Q_stricmp(name, g_pEventHooks->name))
		{
			g_pEventHooks = g_pEventHooks->next;
			if (!g_pEventHooks)
			{
				evhook = (event_hook_t*)Mem_ZeroMalloc(0xCu);
				evhook->name = Mem_Strdup(name);
				evhook->pfnEvent = pfnEvent;
				eventhooks = g_pEventHooks;
				g_pEventHooks = evhook;
				evhook->next = eventhooks;
			}
		}

		Con_DPrintf("CL_HookEvent:  Called on existing hook, updating event hook\n");
		g_pEventHooks->pfnEvent = pfnEvent;
	}
	else
	{
		evhook = (event_hook_t*)Mem_ZeroMalloc(sizeof(event_hook_t));
		evhook->name = Mem_Strdup(name);
		evhook->pfnEvent = pfnEvent;
		eventhooks = g_pEventHooks;
		g_pEventHooks = evhook;
		evhook->next = eventhooks;
	}
}

void CL_QueueEvent(int flags, int index, float delay, event_args_t* pargs)
{
	// TODO: implement - Solokiller
}

void CL_ResetEvent(event_info_t* ei)
{
	ei->index = 0;
	Q_memset(&ei->args, 0, sizeof(ei->args));
	ei->fire_time = 0;
	ei->flags = 0;
}

void CL_Parse_StopSound()
{
	S_StopSound(MSG_ReadShort() >> 3, MSG_ReadShort() & 7);
}


void CL_Parse_CDTrack()
{
	/*
	cl.cdtrack = MSG_ReadByte();
	cl.looptrack = MSG_ReadByte();

	if (!COM_CheckParm("-nocdaudio"))
	{
		if (cls.demoplayback)
		{
			if (cls.forcetrack == -1)
			{
			PlayCDAudio:
				CDAudio_Play(cl.cdtrack, 1);
				return;
			}
		}
		else
		{
			if (cls.demorecording == false)
				goto PlayCDAudio;
			if (cls.forcetrack == -1)
				goto PlayCDAudio;
		}

		CDAudio_Play(cls.forcetrack, 1);
	}
	*/
	return;
}