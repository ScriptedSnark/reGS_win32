//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================
#if !defined(CD_INTERNAL_H)
#define CD_INTERNAL_H
#ifdef _WIN32
#pragma once
#endif

class CCDAudio: public ICDAudio
{
	static const float TRACK_EXTRA_TIME;

public:
	CCDAudio(void);
	virtual ~CCDAudio(void);

	int Init(void) override;
	void Shutdown(void) override;

	void Play(int track, bool looping) override;
	void PlayFile(const char* filename, bool looping) override;
	void Pause(void) override;
	void Resume(void) override;
	void Frame(void) override;

	void MP3_Resume_Audio(void) override;
	void MP3_Suspend_Audio(void) override;

	void CD_f(void);

	enum
	{
		MAX_REMAP = 100
	};

	struct _MP3_AUDIO_STATE
	{
		bool inuse;
		bool suspended;
		bool playing;

		char trackname[512];

		int tracknum;

		bool looping;

		float volume;
	};

	float m_flPlayTime;
	double m_dStartTime;
	double m_dPauseTime;
	double m_dFadeOutTime;

	bool m_bIsCDValid;
	bool m_bIsPlaying;

	bool m_bWasPlaying;
	bool m_bInitialized;
	bool m_bEnabled;
	bool m_bIsLooping;
	bool m_bIsPrimed;
	bool m_bIsInMiddleOfPriming;

	float m_flVolume;
	float m_flMP3Volume;

	int m_nPlayTrack;

	char m_szPendingPlayFilename[512];

	int m_nMaxCDTrack;

	bool m_bResumeOnSwitch;
	unsigned int m_uiDeviceID;
	int m_rgRemapCD[MAX_REMAP];

	_MP3_AUDIO_STATE m_MP3;

	void ResetCDTimes(void);
	void Stop(void);
	void Eject(void);
	void CloseDoor(void);
	void GetAudioDiskInfo(void);
	void SwitchToLauncher(void);
	void SwitchToEngine(void);
	void Reset(void);
	void FadeOut(void);
	void PrimeTrack(char* filename, int looping);

	bool MP3_Init(void);
	void MP3_Shutdown(void);
	void MP3_StartStream(void);
	void MP3_Loop(void);
	void MP3_SetPause(bool OnOff);
	float MP3_SetVolume(float NewVol);
	bool MP3_PlayTrack(int trackNum, bool looping);
	bool MP3_PlayTrack(const char* filename, bool looping);
	void MP3_PlayTrackFinalize(int trackNum, bool looping);
	void MP3_ReleaseDriver(void);
	void MP3_StopStream(void);
	bool MP3_InitStream(int trackNum, bool looping);
	bool MP3_InitStream(const char* filename, bool looping);

	void _PrimeTrack(int track, int looping);

private:
	void _Stop(int, int);
	void _Pause(int, int);
	void _Eject(int, int);
	void _CloseDoor(int, int);
	void _GetAudioDiskInfo(int, int);
	void _Play(int track, int looping);
	void _Resume(int, int);
	void _SwitchToLauncher(int, int);
	void _SwitchToEngine(int, int);
	void _CDUpdate(int, int);
	void _CDReset(int, int);
	void _Init(int, int);
};

CCDAudio* GetInteralCDAudio(void);

#endif // CD_INTERNAL_H
