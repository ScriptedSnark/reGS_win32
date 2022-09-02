#include "quakedef.h"
#include "client.h"
#include "r_studio.h"
#include "gl_model.h"

// TODO: implement and add functions here - Solokiller
engine_studio_api_t engine_studio_api;

// TODO: implement and add functions here - Solokiller
r_studio_interface_t studio;
r_studio_interface_t* pStudioAPI = &studio;

cvar_t* cl_righthand = nullptr;

void R_ResetStudio()
{
	pStudioAPI = &studio;
}

int ModelFrameCount(model_t* model)
{
	// TODO: implement - ScriptedSnark
	return 0;
}

sfx_t* CL_LookupSound(const char* pName)
{
	for (auto pSound : cl.sound_precache)
	{
		if (pSound && !Q_strcmp(pName, pSound->name))
			return pSound;
	}

	return S_PrecacheSound((char*)pName);
}
