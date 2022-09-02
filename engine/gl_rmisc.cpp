#include "quakedef.h"
#include "qgl.h"
#include "render.h"
#include "gl_rmisc.h"
#include "gl_vidnt.h"

qboolean filterMode;
float filterColorRed = 1.0, filterColorGreen = 1.0, filterColorBlue = 1.0;
float filterBrightness = 1.0;

cvar_t gl_clear = {"gl_clear", "0"};

void GL_Dump_f()
{
	if (gl_vendor)
		Con_Printf("GL Vendor: %s\n", gl_vendor);
	if (gl_renderer)
		Con_Printf("GL Renderer: %s\n", gl_renderer);
	if (gl_version)
		Con_Printf("GL Version: %s\n", gl_version);
	if (gl_extensions)
		Con_Printf("GL Extensions: %s\n", gl_extensions);
}

void R_InitTextures()
{
	// TODO: implement - Solokiller
}

void R_Init()
{
	// TODO: implement - Solokiller
	Cmd_AddCommand("gl_dump", GL_Dump_f);
}

void D_FlushCaches()
{
	// Nothing
}
