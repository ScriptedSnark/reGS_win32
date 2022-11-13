//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================
#if !defined(ITHREAD_H)
#define ITHREAD_H
#ifdef _WIN32
#pragma once
#endif

using vfunc = void (CCDAudio::*)(int, int);

class IThread
{
public:
	virtual ~IThread(void) {}
	virtual bool Init(void) = 0;
	virtual bool Shutdown(void) = 0;
	virtual bool AddThreadItem(vfunc pfn, int param1, int param2) = 0;
};

extern IThread* thread;

#endif // ITHREAD_H