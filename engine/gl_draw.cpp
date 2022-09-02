#include "quakedef.h"
#include "cdll_int.h"
#include "decals.h"
#include "gl_draw.h"
#include "gl_rmain.h"
#include "gl_screen.h"
#include "gl_vidnt.h"
#include "qgl.h"
#include "wad.h"

#include "vgui2/text_draw.h"

#include <vid.h>

int g_currentpalette;
cvar_t gl_ansio = {"gl_ansio", "16"};

qpic_t* draw_disc = nullptr;

#define MAX_CACHED_PICS 128

typedef struct
{
	int texnum;
	short servercount;
	short paletteIndex;
	int width;
	int height;
	qboolean mipmap;
	char identifier[64];
} gltexture_t;

static gltexture_t gltextures[MAX_GLTEXTURES];

struct GL_PALETTE
{
	int tag;
	int referenceCount;
	unsigned __int8 colors[768];
};

GL_PALETTE gGLPalette[350];
static int numgltextures = 0;

static int scissor_x = 0, scissor_y = 0, scissor_width = 0, scissor_height = 0;
static qboolean giScissorTest = false;

void EnableScissorTest(int x, int y, int width, int height)
{
	x = clamp(x, 0, vid.width);
	y = clamp(y, 0, vid.height);
	width = clamp(width, 0, vid.width - x);
	height = clamp(height, 0, vid.height - y);

	scissor_x = x;
	scissor_width = width;
	scissor_y = y;
	scissor_height = height;

	giScissorTest = true;
}

void DisableScissorTest(void)
{
	scissor_x = 0;
	scissor_width = 0;
	scissor_y = 0;
	scissor_height = 0;

	giScissorTest = false;
}

void Draw_Init()
{
	m_bDrawInitialized = true;
	VGUI2_Draw_Init();

	// TODO: implement - Solokiller

	Cvar_RegisterVariable(&gl_ansio);

	if (Host_GetVideoLevel() > 0)
	{
		Cvar_DirectSet(&gl_ansio, "4");
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_ansio.value);
	}
	// TODO: implement - Solokiller
}

void Draw_FillRGBA(int x, int y, int w, int h, int r, int g, int b, int a)
{
	g_engdstAddrs.pfnFillRGBA(&x, &y, &w, &h, &r, &g, &b, &a);

	qglDisable(GL_TEXTURE_2D);
	qglEnable(GL_BLEND);

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglBlendFunc(GL_SRC_ALPHA, GL_LINES);
	qglColor4f(r / 255.0, g / 255.0, b / 255.0, a / 255.0);

	qglBegin(GL_RELATIVE_HORIZONTAL_LINE_TO_NV);
	qglVertex2f(x, y);
	qglVertex2f(w + x, y);
	qglVertex2f(w + x, h + y);
	qglVertex2f(x, h + y);
	qglEnd();

	qglColor3f(1.0, 1.0, 1.0);
	qglEnable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
}

int Draw_Character(int x, int y, int num, unsigned int font)
{
	return VGUI2_Draw_Character(x, y, num, font);
}

int Draw_MessageCharacterAdd(int x, int y, int num, int rr, int gg, int bb, unsigned int font)
{
	return VGUI2_Draw_CharacterAdd(x, y, num, rr, gg, bb, font);
}

int Draw_String(int x, int y, char* str)
{
	int width = VGUI2_DrawString(x, y, str, VGUI2_GetConsoleFont());
	Draw_ResetTextColor();

	return width + x;
}

int Draw_StringLen(const char* psz, unsigned int font)
{
	return VGUI2_Draw_StringLen(psz, font);
}

void Draw_SetTextColor(float r, float g, float b)
{
	g_engdstAddrs.pfnDrawSetTextColor(&r, &g, &b);

	VGUI2_Draw_SetTextColor(
		static_cast<int>(r * 255.0),
		static_cast<int>(g * 255.0),
		static_cast<int>(b * 255.0));
}

void Draw_GetDefaultColor()
{
	int r, g, b;

	if (sscanf(con_color.string, "%i %i %i", &r, &g, &b) == 3)
		VGUI2_Draw_SetTextColor(r, g, b);
}

void Draw_ResetTextColor()
{
	Draw_GetDefaultColor();
}

void Draw_FillRGBABlend(int x, int y, int w, int h, int r, int g, int b, int a)
{
	g_engdstAddrs.pfnFillRGBA(&x, &y, &w, &h, &r, &g, &b, &a);

	qglDisable(GL_TEXTURE_2D);
	qglEnable(GL_BLEND);

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglColor4f(r / 255.0, g / 255.0, b / 255.0, a / 255.0);

	qglBegin(GL_RELATIVE_HORIZONTAL_LINE_TO_NV);
	qglVertex2f(x, y);
	qglVertex2f(w + x, y);
	qglVertex2f(w + x, h + y);
	qglVertex2f(x, h + y);
	qglEnd();

	qglColor3f(1.0, 1.0, 1.0);

	qglEnable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
}

GLuint GL_GenTexture()
{
	GLuint tex;

	qglGenTextures(1, &tex);

	return tex;
}

GLenum oldtarget = TEXTURE0_SGIS;

void GL_UnloadTexture(char* identifier)
{
	int idx;
	if (numgltextures > 0)
	{
		do
		{
			if (Q_strcmp(identifier, gltextures->identifier) == 0)
			{
				if (((gltextures->servercount + 1) & MAX_CACHED_PICS) != 0)
				{
					return;
				}
				qglDeleteTextures(1, (GLuint*)gltextures);
				if (gltextures->paletteIndex > -1)
				{
					if (gGLPalette[gltextures->paletteIndex].referenceCount < 2)
					{
						if (gltextures->paletteIndex < 350)
						{
							gGLPalette[gltextures->paletteIndex].tag = -1;
							gGLPalette[gltextures->paletteIndex].referenceCount = 0;
						}
					}
					else
					{
						gGLPalette[gltextures->paletteIndex].referenceCount -= 1;
					}
				}
				// There should be "dest" variable with "RtlSecureZeroMemory(dest, sizeof(gltexture_t));", but dest is useless here (???) - xWhitey
				gltextures->servercount = -1; // ((DWORD) dest) + 2 == dest->servercount
			}
			idx += 1;
		} while (idx < numgltextures);
	}
}

void GL_SelectTexture(GLenum target)
{
	if (!gl_mtexable)
		return;

	qglSelectTextureSGIS(target);

	if (target == oldtarget)
		return;

	cnttextures[oldtarget - TEXTURE0_SGIS] = currenttexture;
	currenttexture = cnttextures[target - TEXTURE0_SGIS];
	oldtarget = target;
}

void GL_PaletteSelect(int paletteIndex)
{
	if (g_currentpalette != paletteIndex)
	{
		if (qglColorTableEXT)
		{
			g_currentpalette = paletteIndex;
			qglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, GL_DYNAMIC_STORAGE_BIT, GL_RGB, GL_UNSIGNED_BYTE, gGLPalette[paletteIndex].colors);
		}
	}
}

void GL_Bind(int texnum)
{
	if (currenttexture == texnum)
		return;

	int i = (texnum >> 16) - 1;

	currenttexture = texnum;
	qglBindTexture(GL_TEXTURE_2D, texnum);

	if (i >= 0)
		GL_PaletteSelect(i);
}

bool ValidateWRect(const wrect_t* prc)
{
	if (prc)
	{
		if ((prc->left <= prc->right) || (prc->top >= prc->bottom))
			return false;
	}
	else
		return false;

	return true;
}

bool IntersectWRect(const wrect_t* prc1, const wrect_t* prc2, wrect_t* prc)
{
	wrect_t rc;

	if (!prc)
		prc = &rc;

	prc->left = max(prc1->left, prc2->left);
	prc->right = min(prc1->right, prc2->right);

	if (prc->left < prc->right)
	{
		prc->top = max(prc1->top, prc2->top);
		prc->bottom = min(prc1->bottom, prc2->bottom);

		if (prc->top < prc->bottom)
			return true;
	}

	return false;
}

void AdjustSubRect(mspriteframe_t* pFrame, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom, int* pw, int* ph, const wrect_t* prcSubRect)
{
	wrect_t wrect;

	if (!ValidateWRect(prcSubRect))
		return;

	wrect.left = 0;
	wrect.top = 0;
	wrect.right = *pw;
	wrect.bottom = *ph;

	if (!IntersectWRect(prcSubRect, &wrect, &wrect))
		return;

	*pw = wrect.right - wrect.left;
	*ph = wrect.bottom - wrect.top;

	*pfLeft = (wrect.left) * 1.0 / pFrame->width;
	*pfRight = (wrect.right) * 1.0 / pFrame->width;

	*pfTop = (wrect.top) * 1.0 / pFrame->height;
	*pfBottom = (wrect.bottom) * 1.0 / pFrame->height;
}

void Draw_Frame(mspriteframe_t* pFrame, int ix, int iy, const wrect_t* prcSubRect)
{
	float fLeft = 0.0;
	float fRight = 1.0;
	float fTop = 0.0;
	float fBottom = 1.0;

	int iWidth = pFrame->width;
	int iHeight = pFrame->height;

	float x = (float)ix;
	float y = (float)iy;

	VGUI2_ResetCurrentTexture();

	if (giScissorTest)
	{
		qglScissor(scissor_x, scissor_y, scissor_width, scissor_height);
		qglEnable(GL_SCISSOR_TEST);
	}

	if (prcSubRect)
		AdjustSubRect(pFrame, &fLeft, &fRight, &fTop, &fBottom, &iWidth, &iHeight, prcSubRect);

	qglDepthMask(GL_FALSE);

	GL_Bind(pFrame->gl_texturenum);

	qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	qglBegin(GL_QUADS);
	qglTexCoord2f(fLeft, fTop);
	qglVertex2f(x, y);
	qglTexCoord2f(fRight, fTop);
	qglVertex2f(iWidth + x, y);
	qglTexCoord2f(fRight, fBottom);
	qglVertex2f(iWidth + x, iHeight + y);
	qglTexCoord2f(fLeft, fBottom);
	qglVertex2f(x, iHeight + y);
	qglEnd();

	qglDepthMask(GL_TRUE);
	qglDisable(GL_SCISSOR_TEST);
}

void Draw_SpriteFrame(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect)
{
	Draw_Frame(pFrame, x, y, prcSubRect);
}

void Draw_SpriteFrameHoles(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect)
{
	qglEnable(GL_ALPHA_TEST);

	/* - TODO: implement gl_spriteblend - ScriptedSnark
	if (gl_spriteblend.value != 0.0)
	{
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglEnable(GL_BLEND);
	}
	*/

	Draw_Frame(pFrame, x, y, prcSubRect);
	qglDisable(GL_ALPHA_TEST);
	qglDisable(GL_BLEND);
}

void Draw_SpriteFrameAdditive(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect)
{
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);
	Draw_Frame(pFrame, x, y, prcSubRect);
	qglDisable(GL_BLEND);
}

void Draw_SpriteFrameGeneric(mspriteframe_t* pFrame, unsigned short* pPalette, int x, int y, const wrect_t* prcSubRect, int src, int dest, int width, int height)
{
	int w, h;

	w = pFrame->width;
	pFrame->width = width;
	h = pFrame->height;
	pFrame->height = height;

	qglEnable(GL_BLEND);
	qglBlendFunc(src, dest);
	Draw_Frame(pFrame, x, y, prcSubRect);
	qglDisable(GL_BLEND);

	pFrame->width = w;
	pFrame->height = h;
}

void Draw_Pic(int x, int y, qpic_t* pic)
{
	int tempVariable;
	if (pic != NULL)
	{
		VGUI2_ResetCurrentTexture();
		qglEnable(GL_TEXTURE_2D);
		qglDisable(GL_BLEND);
		qglDisable(GL_DEPTH_TEST);
		qglEnable(GL_ALPHA_TEST);
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		qglColor4f(1.0, 1.0, 1.0, 1.0);
		if (*pic->data != currenttexture)
		{
			tempVariable = (*pic->data >> 16) - 1;
			currenttexture = *pic->data;
			qglBindTexture(GL_TEXTURE_2D, *pic->data);
			if (tempVariable >= 0 && tempVariable != g_currentpalette)
			{
				g_currentpalette = tempVariable;
				qglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, GL_DYNAMIC_STORAGE_BIT, GL_RGB, GL_UNSIGNED_BYTE, gGLPalette[tempVariable].colors);
			}
		}
		qglBegin(GL_QUADS);
		qglTexCoord2f(pic[1].width, pic[1].height);
		qglVertex2f(x, y);
		qglTexCoord2f(*pic[1].data, pic[1].height);
		qglVertex2f(pic->width + x, y);
		qglTexCoord2f(*pic[1].data, pic[2].width);
		qglVertex2f(pic->width + x, pic->height + y);
		qglTexCoord2f(pic[1].width, pic[2].width);
		qglVertex2f(x, pic->height + y);
	}
}

void Draw_BeginDisc()
{
	if (draw_disc)
		Draw_CenterPic(draw_disc);
}
