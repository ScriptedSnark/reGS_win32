#include "quakedef.h"
#include "client.h"
#include "cl_draw.h"
#include "gl_model.h"
#include "render.h"
#include <vid.h>
#include <cl_tent.h>
#include <gl_draw.h>

#define SPR_MAX_SPRITES 256
unsigned short gSpritePalette[256];

HSPRITE ghCrosshair = 0;
wrect_t gCrosshairRc = {};
int gCrosshairR = 0;
int gCrosshairG = 0;
int gCrosshairB = 0;

static msprite_t* gpSprite = nullptr;

static int gSpriteCount = 0;
static SPRITELIST* gSpriteList = nullptr;

msprite_t* SPR_Pointer(SPRITELIST* pList)
{
	return (msprite_t*)pList->pSprite->cache.data;
}

void SetCrosshair(HSPRITE hspr, wrect_t rc, int r, int g, int b)
{
	g_engdstAddrs.pfnSetCrosshair(&hspr, &rc, &r, &g, &b);

	ghCrosshair = hspr;
	gCrosshairRc.left = rc.left;
	gCrosshairRc.right = rc.right;
	gCrosshairRc.top = rc.top;
	gCrosshairRc.bottom = rc.bottom;
	gCrosshairR = r;
	gCrosshairG = g;
	gCrosshairB = b;
}

void DrawCrosshair(int x, int y)
{
	if (ghCrosshair)
	{
		SPR_Set(ghCrosshair, gCrosshairR, gCrosshairG, gCrosshairB);

		SPR_DrawHoles(
			0,
			x - (gCrosshairRc.right - gCrosshairRc.left) / 2,
			y - (gCrosshairRc.bottom - gCrosshairRc.top) / 2,
			&gCrosshairRc);
	}
}

void SPR_Init()
{
	if (!gSpriteList)
	{
		ghCrosshair = 0;
		gSpriteCount = SPR_MAX_SPRITES;
		gSpriteList = reinterpret_cast<SPRITELIST*>(Mem_ZeroMalloc(SPR_MAX_SPRITES * sizeof(SPRITELIST)));
		gpSprite = nullptr;
	}
}

void SPR_Shutdown()
{
	int idx;

	if (host_initialized)
	{
		if (gSpriteList)
		{
			if (gSpriteCount > 0)
			{
				idx = 0;
				do
				{
					if (gSpriteList->pSprite)
					{
						Mod_UnloadSpriteTextures(gSpriteList[idx].pSprite);
					}
					if (gSpriteList[idx].pName)
					{
						Mem_Free(gSpriteList[idx].pName);
					}
					++idx;
				} while (idx < gSpriteCount);
			}
		}
		gpSprite = nullptr;
		gSpriteList = nullptr;
		gSpriteCount = 0;
		ghCrosshair = 0;
	}
	return;
}

void SPR_Shutdown_NoModelFree()
{
	SPRITELIST* p;
	int idx;

	if (host_initialized)
	{
		p = gSpriteList;
		if (gSpriteList)
		{
			if (gSpriteCount > 0)
			{
				idx = 0;
				do
				{
					if (gSpriteList[idx].pName)
					{
						Mem_Free(gSpriteList[idx].pName);
					}
					++idx;
				} while (idx < gSpriteCount);
			}
		}
		gSpriteList = nullptr;
		gpSprite = nullptr;
		gSpriteCount = 0;
		ghCrosshair = 0;
	}
}

HSPRITE SPR_Load(const char* pTextureName)
{
	int idx;

	g_engdstAddrs.pfnSPR_Load(&pTextureName);
	if (pTextureName && gSpriteList && gSpriteCount > 0)
	{
		idx = 0;
		while (true)
		{
			if (!gSpriteList[idx].pSprite)
			{
				gSpriteList[idx].pName = (char*)Mem_Malloc(Q_strlen(pTextureName) + 1);
				Q_strcpy(gSpriteList[idx].pName, pTextureName);
			}
			if (!Q_stricmp(pTextureName, gSpriteList[idx].pName))
				break;

			++idx;

			if (gSpriteCount <= idx)
				Sys_Error("cannot allocate more than 256 HUD sprites\n");
		}
		// gSpriteMipMap = false;
		gSpriteList[idx].pSprite = Mod_ForName(pTextureName, false, true);
		// gSpriteMipMap = true;
		if (gSpriteList[idx].pSprite)
		{
			// gSpriteList[idx].frameCount = ModelFrameCount(gSpriteList[idx].pSprite);
		}
	}

	return 0;
}

int SPR_Frames(HSPRITE hSprite)
{
	int result;
	SPRITELIST* sprlist;

	result = 0;
	g_engdstAddrs.pfnSPR_Frames(&hSprite);
	hSprite--;

	if (hSprite < 0 || hSprite >= gSpriteCount)
		return result;

	sprlist = &gSpriteList[hSprite];

	if (sprlist)
		result = sprlist->frameCount;

	return result;
}

int SPR_Height(HSPRITE hSprite, int frame)
{
	int result;
	SPRITELIST* sprlist;
	mspriteframe_t* sprframe;

	result = 0;
	g_engdstAddrs.pfnSPR_Height(&hSprite, &frame);
	hSprite--;

	if (hSprite < 0 || hSprite >= gSpriteCount)
		return result;

	sprlist = &gSpriteList[hSprite];

	if (sprlist)
	{
		sprframe = R_GetSpriteFrame(SPR_Pointer(sprlist), frame);
		if (sprframe)
			result = sprframe->height;
	}

	return result;
}

int SPR_Width(HSPRITE hSprite, int frame)
{
	int result;
	SPRITELIST* sprlist;
	mspriteframe_t* sprframe;

	result = 0;
	g_engdstAddrs.pfnSPR_Width(&hSprite, &frame);
	hSprite--;

	if (hSprite < 0 || hSprite >= gSpriteCount)
		return result;

	sprlist = &gSpriteList[hSprite];
	if (sprlist)
	{
		sprframe = R_GetSpriteFrame(SPR_Pointer(sprlist), frame);
		if (sprframe)
			result = sprframe->width;
	}
	return result;
}

void SPR_Set(HSPRITE hSprite, int r, int g, int b)
{
	SPRITELIST* sprlist = nullptr;

	g_engdstAddrs.pfnSPR_Set(&hSprite, &r, &g, &b);
	hSprite--;

	if (hSprite < 0 || hSprite >= gSpriteCount)
		return;

	sprlist = &gSpriteList[hSprite];

	if (sprlist)
	{
		gpSprite = SPR_Pointer(sprlist);
		if (gpSprite)
		{
			qglColor4f(r / 255.0, g / 255.0, b / 255.0, 1.0);
		}
	}
}

void SPR_EnableScissor(int x, int y, int width, int height)
{
	g_engdstAddrs.pfnSPR_EnableScissor(&x, &y, &width, &height);
	EnableScissorTest(x, y, width, height);
}

void SPR_DisableScissor()
{
	g_engdstAddrs.pfnSPR_DisableScissor();
	DisableScissorTest();
}

void SPR_Draw(int frame, int x, int y, const wrect_t* prc)
{
	mspriteframe_t* spr;

	g_engdstAddrs.pfnSPR_Draw(&frame, &x, &y, &prc);

	if (gpSprite && vid.width > x && vid.height > y)
	{
		spr = R_GetSpriteFrame(gpSprite, frame);
		if (spr)
			Draw_SpriteFrame(spr, gSpritePalette, x, y, prc);
		else
			Con_DPrintf("Client.dll SPR_Draw error:  invalid frame\n");
	}
}

void SPR_DrawHoles(int frame, int x, int y, const wrect_t* prc)
{
	mspriteframe_t* sprframe;
	g_engdstAddrs.pfnSPR_DrawHoles(&frame, &x, &y, &prc);

	if (gpSprite && vid.width > x && vid.height > y)
	{
		sprframe = R_GetSpriteFrame(gpSprite, frame);

		if (sprframe)
			Draw_SpriteFrameHoles(sprframe, gSpritePalette, x, y, prc);
		else
			Con_DPrintf("Client.dll SPR_DrawHoles error:  invalid frame\n");
	}
}

void SPR_DrawAdditive(int frame, int x, int y, const wrect_t* prc)
{
	mspriteframe_t* sprframe;
	g_engdstAddrs.pfnSPR_DrawAdditive(&frame, &x, &y, &prc);

	if (gpSprite && vid.width > x && vid.height > y)
	{
		sprframe = R_GetSpriteFrame(gpSprite, frame);

		if (sprframe)
			Draw_SpriteFrameAdditive(sprframe, gSpritePalette, x, y, prc);
		else
			Con_DPrintf("Client.dll SPR_DrawAdditive error:  invalid frame\n");
	}
}

void SPR_DrawGeneric(int frame, int x, int y, const wrect_t* prc, int src, int dest, int width, int height)
{
	mspriteframe_t* sprframe;
	g_engdstAddrs.pfnSPR_DrawGeneric(&frame, &x, &y, &prc, &src, &dest, &width, &height);

	if (gpSprite && vid.width > x && vid.height > y)
	{
		sprframe = R_GetSpriteFrame(gpSprite, frame);

		if (sprframe)
			Draw_SpriteFrameGeneric(sprframe, gSpritePalette, x, y, prc, src, dest, width, height);
		else
			Con_DPrintf("Client.dll SPR_DrawGeneric error: invalid frame\n");
	}
}

client_sprite_t* SPR_GetList(char* psz, int* piCount)
{
	g_engdstAddrs.pfnSPR_GetList(&psz, &piCount);

	auto pszData = reinterpret_cast<char*>(COM_LoadTempFile(psz, nullptr));

	if (pszData)
	{
		pszData = COM_Parse(pszData);
		const auto iNumSprites = atoi(com_token);

		if (iNumSprites)
		{
			auto pList = reinterpret_cast<client_sprite_t*>(calloc(sizeof(client_sprite_t) * iNumSprites, 1));

			if (pList)
			{
				auto pSprite = pList;

				for (int i = 0; i < iNumSprites; ++i, ++pSprite)
				{
					pszData = COM_Parse(pszData);
					Q_strncpy(pSprite->szName, com_token, ARRAYSIZE(pSprite->szName));

					pszData = COM_Parse(pszData);
					pSprite->iRes = atoi(com_token);

					pszData = COM_Parse(pszData);
					Q_strncpy(pSprite->szSprite, com_token, ARRAYSIZE(pSprite->szSprite));

					pszData = COM_Parse(pszData);
					pSprite->rc.left = atoi(com_token);

					pszData = COM_Parse(pszData);
					pSprite->rc.top = atoi(com_token);

					pszData = COM_Parse(pszData);
					pSprite->rc.right = pSprite->rc.left + atoi(com_token);

					pszData = COM_Parse(pszData);
					pSprite->rc.bottom = pSprite->rc.top + atoi(com_token);
				}

				if (piCount)
					*piCount = iNumSprites;
			}

			return pList;
		}
	}

	return nullptr;
}

SPRITELIST* SPR_Get(HSPRITE hSprite)
{
	const int iIndex = hSprite - 1;

	if (iIndex >= 0 && iIndex < gSpriteCount)
		return &gSpriteList[iIndex];

	return nullptr;
}

model_t* SPR_GetModelPointer(HSPRITE hSprite)
{
	auto pList = SPR_Get(hSprite);

	if (pList)
		return pList->pSprite;

	return nullptr;
}

void SetFilterMode(int mode)
{
	g_engdstAddrs.pfnSetFilterMode(&mode);
	filterMode = mode;
}

void SetFilterColor(float r, float g, float b)
{
	g_engdstAddrs.pfnSetFilterColor(&r, &g, &b);
	filterColorRed = r;
	filterColorGreen = g;
	filterColorBlue = b;
}

void SetFilterBrightness(float brightness)
{
	g_engdstAddrs.pfnSetFilterBrightness(&brightness);
	filterBrightness = brightness;
}
