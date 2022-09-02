/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/

// sys_engine.cpp

//#include "winquake.h"
#include "sys.h"
#include "cdll_int.h"
#include "dll_state.h"

#include "host.h"
#include "IEngine.h"
#include "keys.h"
#include "IGame.h"

#define MINIMIZED_SLEEP 20
#define NOT_FOCUS_SLEEP 50 // sleep time when not focus

void GameSetState(int iState);
void GameSetSubState(int iSubState);
void Dispatch_Substate(int iSubState);

void Sys_ShutdownGame(void);
bool Sys_InitGame(char* lpOrgCmdLine, char* pBaseDir, void* pwnd, bool bIsDedicated);

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CEngine: public IEngine
{
public:
	CEngine(void);
	virtual ~CEngine(void);

	bool Load(bool dedicated, char* basedir, char* cmdline);
	void Unload(void);

	void SetState(int iState);
	int GetState(void);

	void SetSubState(int iSubState);
	int GetSubState(void);

	int Frame(void);
	double GetFrameTime(void);
	double GetCurTime(void);

	void TrapKey_Event(int key, bool down);
	void TrapMouse_Event(int buttons, bool down);
	void StartTrapMode(void);
	bool IsTrapping(void);
	bool CheckDoneTrapping(int& buttons, int& key);

	int GetQuitting(void);
	void SetQuitting(int quittype);

private:
	int m_nQuitting;
	int m_nDLLState;
	int m_nSubState;

	double m_fCurTime;
	double m_fFrameTime;
	double m_fOldTime;

	bool m_bTrapMode;
	bool m_bDoneTrapping;
	int m_nTrapKey;
	int m_nTrapButtons;
};

static CEngine g_Engine;

IEngine* eng = &g_Engine;
// IEngineAPI *engine = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEngine::CEngine(void)
{
	m_fFrameTime = 0.0f;
	m_nSubState = 0;

	m_nDLLState = DLL_INACTIVE;

	m_fOldTime = 0.0f;

	m_bTrapMode = false;
	m_bDoneTrapping = false;
	m_nTrapKey = 0;
	m_nTrapButtons = 0;

	m_nQuitting = QUIT_NOTQUITTING;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEngine::~CEngine(void)
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEngine::Unload(void)
{
	Sys_ShutdownGame();

	m_nDLLState = DLL_INACTIVE;
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::Load(bool dedicated, char* basedir, char* cmdline)
{
	bool success = false;

	// Activate engine
	SetState(DLL_ACTIVE);

	if (Sys_InitGame(
			cmdline,
			basedir,
			game->GetMainWindow(),
			dedicated))
	{
		success = true;
	}

	if (success)
	{
		ForceReloadProfile();
	}

	return success;
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : int
//-----------------------------------------------------------------------------
int CEngine::Frame(void)
{
	// Update CD & mp3
	//cdaudio->Frame();

	// yield the CPU for a little while when paused, minimized, or not the focus
	// FIXME:  Move this to main windows message pump?
	if (!game->IsActiveApp())
	{
		if (m_nDLLState == DLL_PAUSED)
			game->SleepUntilInput(NOT_FOCUS_SLEEP);
		else
			game->SleepUntilInput(MINIMIZED_SLEEP);
	}

	m_fCurTime = Sys_FloatTime();

	// Set frame time
	m_fFrameTime = m_fCurTime - m_fOldTime;

	// Remember old time
	m_fOldTime = m_fCurTime;

	// If the time is < 0, that means we've restarted.
	// Set the new time high enough so the engine will run a frame
	if (m_fFrameTime < 0.0)
		m_fFrameTime = 0.01;

	if (m_nDLLState)
	{
		// Run the engine frame
		int dummy;
		int iState = Host_Frame(m_fFrameTime, m_nDLLState, &dummy);

		// Has the state changed?
		if (iState != m_nDLLState)
		{
			SetState(iState);

			switch (m_nDLLState)
			{
			case DLL_INACTIVE:
				break;

			case DLL_CLOSE:
				SetQuitting(QUIT_TODESKTOP);
				break;

			case DLL_STATE_RESTART:
				SetQuitting(QUIT_RESTART);
				break;
			}
		}
	}

	return (m_nDLLState);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEngine::SetSubState(int iSubState)
{
	if (iSubState != 1)
		GameSetSubState(iSubState);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : iState -
//-----------------------------------------------------------------------------
void CEngine::SetState(int iState)
{
	m_nDLLState = iState;
	GameSetState(iState);
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : int
//-----------------------------------------------------------------------------
int CEngine::GetState(void)
{
	return m_nDLLState;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEngine::GetSubState(void)
{
	return m_nSubState;
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : double
//-----------------------------------------------------------------------------
double CEngine::GetFrameTime(void)
{
	return m_fFrameTime;
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : double
//-----------------------------------------------------------------------------
double CEngine::GetCurTime(void)
{
	return m_fCurTime;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : key -
//			down -
//-----------------------------------------------------------------------------
void CEngine::TrapKey_Event(int key, bool down)
{
	// Only key down events are trapped
	if (m_bTrapMode && down)
	{
		m_bTrapMode = false;
		m_bDoneTrapping = true;

		m_nTrapKey = key;
		m_nTrapButtons = 0;
		return;
	}

	Key_Event(key, down ? 1 : 0);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : buttons -
//			down -
//-----------------------------------------------------------------------------
void CEngine::TrapMouse_Event(int buttons, bool down)
{
	// buttons == 0 for mouse move events
	if (m_bTrapMode && buttons && !down)
	{
		m_bTrapMode = false;
		m_bDoneTrapping = true;

		m_nTrapKey = 0;
		m_nTrapButtons = buttons;
		return;
	}

	ClientDLL_MouseEvent(buttons);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEngine::StartTrapMode(void)
{
	if (m_bTrapMode)
		return;

	m_bDoneTrapping = false;
	m_bTrapMode = true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::IsTrapping(void)
{
	return m_bTrapMode;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : buttons -
//			key -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEngine::CheckDoneTrapping(int& buttons, int& key)
{
	if (m_bTrapMode)
		return false;

	if (!m_bDoneTrapping)
		return false;

	key = m_nTrapKey;
	buttons = m_nTrapButtons;

	// Reset since we retrieved the results
	m_bDoneTrapping = false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Flag that we are in the process of quiting
//-----------------------------------------------------------------------------
void CEngine::SetQuitting(int quittype)
{
	m_nQuitting = quittype;
}

//-----------------------------------------------------------------------------
// Purpose: Check whether we are ready to exit
//-----------------------------------------------------------------------------
int CEngine::GetQuitting(void)
{
	return m_nQuitting;
}
