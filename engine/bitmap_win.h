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

// bitmap_win.h
#if !defined(BITMAP_WIN_H)
#define BITMAP_WIN_H
#pragma once

#include "tier1/KeyValues.h"

void LoadBMP8(FileHandle_t hFile, byte** pPalette, int* nPalette, byte** pImage, int* nWidth, int* nHeight);

#endif // BITMAP_WIN_H
