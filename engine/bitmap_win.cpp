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

// bitmap_win.cpp - BMP Loading

#include <winlite.h>
#include "quakedef.h"

void LoadBMP8(FileHandle_t hFile, byte** pPalette, int* nPalette, byte** pImage, int* nWidth, int* nHeight)
{
	RGBQUAD rgrgbPalette[256];

	BITMAPINFOHEADER bmih;
	BITMAPFILEHEADER bmfh;

	*pImage = nullptr;
	*pPalette = nullptr;

	if (nWidth)
		*nWidth = 0;
	if (nHeight)
		*nHeight = 0;

	if (FS_Read(&bmfh, sizeof(BITMAPFILEHEADER), hFile) == sizeof(BITMAPFILEHEADER) && !bmfh.bfReserved1 && !bmfh.bfReserved2)
	{
		if (bmfh.bfSize <= FS_Size(hFile) && FS_Read(&bmih, sizeof(bmih), hFile) == sizeof(bmih) && bmih.biSize == sizeof(bmih) && bmih.biPlanes == 1 && bmih.biBitCount == 8 && !bmih.biCompression && bmih.biWidth > 0 && bmih.biHeight > 0)
		{
			if (!bmih.biClrUsed)
			{
				bmih.biClrUsed = 256;
			}
			else
			{
				if (bmih.biClrUsed > 256)
					goto GetOut;
			}

			if (FS_Read(rgrgbPalette, sizeof(DWORD) * bmih.biClrUsed, hFile) != sizeof(DWORD) * bmih.biClrUsed)
				goto GetOut;

			ULONG cbBmpBitsa = bmfh.bfSize - FS_Tell(hFile);

			DWORD biTrueWidth = ((bmih.biWidth + sizeof(RGBTRIPLE) & 0xFFFFFFFC));
			DWORD biPixelCount = bmih.biHeight * biTrueWidth;

			if (biPixelCount < biTrueWidth)
				goto GetOut;

			if (biPixelCount >= (DWORD)bmih.biHeight && biPixelCount <= cbBmpBitsa)
			{
				byte* pTempPal = (byte*)Mem_Malloc(768);

				*pPalette = pTempPal;

				if (!pTempPal)
					goto GetOut;

				Q_memset(pTempPal, 0, 768);

				for (DWORD i = 0; i < bmih.biClrUsed; i++)
				{
					pTempPal[0] = rgrgbPalette[i].rgbRed;
					pTempPal[1] = rgrgbPalette[i].rgbGreen;
					pTempPal[2] = rgrgbPalette[i].rgbBlue;

					// 3 colors, each one byte in size.
					pTempPal += sizeof(byte) * sizeof(RGBTRIPLE);
				}

				byte* pOutput = (byte*)Mem_Malloc(cbBmpBitsa);

				if (!pOutput)
				{
					Mem_Free(pTempPal);
					pTempPal = nullptr;

					if (pOutput)
						Mem_Free(pOutput);

					goto GetOut;
				}

				if (FS_Read(pOutput, cbBmpBitsa, hFile) != cbBmpBitsa)
				{
					Mem_Free(pTempPal);
					pTempPal = nullptr;

					if (pOutput)
						Mem_Free(pOutput);

					goto GetOut;
				}

				byte* pTempImage = (byte*)Mem_Malloc(cbBmpBitsa);
				*pImage = pTempImage;

				byte* pCurrOutput = &pOutput[biTrueWidth * (bmih.biHeight - 1)];

				for (int i = 0; i < bmih.biHeight; i++)
				{
					memcpy(pTempImage, pCurrOutput, biTrueWidth);

					pCurrOutput -= biTrueWidth;
					pTempImage += biTrueWidth;
				}

				Mem_Free(&pCurrOutput[biTrueWidth]);

				if (nWidth)
					*nWidth = bmih.biWidth;
				if (nHeight)
					*nHeight = bmih.biHeight;
			}
		}
	}

GetOut:
	if (hFile)
		FS_Close(hFile);
}
