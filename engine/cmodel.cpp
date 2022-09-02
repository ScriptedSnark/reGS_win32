#include "quakedef.h"
#include "cmodel.h"
#include "gl_model.h"

unsigned char* gPAS;
unsigned char* gPVS;
int gPVSRowBytes;
unsigned char mod_novis[MODEL_MAX_PVS];

void Mod_Init()
{
	Q_memset(mod_novis, 255, MODEL_MAX_PVS);
}

byte* Mod_DecompressVis(byte* in, model_t* model)
{
	// #define MAX_MAP_LEAFS 32767
	// i just copypasted this thing from quake - xWhitey
	static byte decompressed[32767 / 8];
	int c;
	byte* out;
	int row;

	row = (model->numleafs + 7) >> 3;
	out = decompressed;

#if 0
	memcpy(out, in, row);
#else
	if (!in)
	{ // no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
#endif

	return decompressed;
}

void CM_DecompressPVS(unsigned char* in, unsigned char* decompressed, int byteCount)
{
	int c;
	unsigned char* out;

	if (in == NULL)
	{
		Q_memcpy(decompressed, mod_novis, byteCount);
		return;
	}

	out = decompressed;
	while (out < decompressed + byteCount)
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;

		if (c > decompressed + byteCount - out)
		{
			c = decompressed + byteCount - out;
		}

		Q_memset(out, 0, c);
		out += c;
	}
}

unsigned char* CM_LeafPVS(int leafnum)
{
	if (gPVS)
	{
		return &gPVS[gPVSRowBytes * leafnum];
	}

	return mod_novis;
}

unsigned char* CM_LeafPAS(int leafnum)
{
	if (gPAS)
	{
		return &gPAS[gPVSRowBytes * leafnum];
	}

	return mod_novis;
}

unsigned char* Mod_LeafPVS(mleaf_t* leaf, model_t* model)
{
	if (leaf == model->leafs)
	{
		return mod_novis;
	}

	if (!gPVS)
	{
		return Mod_DecompressVis(leaf->compressed_vis, model);
	}

	int leafnum = leaf - model->leafs;
	return CM_LeafPVS(leafnum);
}

void CM_FreePAS()
{
	if (gPAS)
		Mem_Free(gPAS);
	if (gPVS)
		Mem_Free(gPVS);
	gPAS = 0;
	gPVS = 0;
}

void CM_CalcPAS(model_t* pModel)
{
	int rows, rowwords;
	int actualRowBytes;
	int i, j, k, l;
	int index;
	int bitbyte;
	unsigned int *dest, *src;
	unsigned char* scan;
	int count, vcount, acount;

	Con_DPrintf("Building PAS...\n");
	CM_FreePAS();

	rows = (pModel->numleafs + 7) / 8;
	count = pModel->numleafs + 1;
	actualRowBytes = (rows + 3) & 0xFFFFFFFC;
	rowwords = actualRowBytes / 4;
	gPVSRowBytes = actualRowBytes;

	gPVS = (byte*)Mem_Calloc(gPVSRowBytes, count);

	scan = gPVS;
	vcount = 0;
	for (i = 0; i < count; i++, scan += gPVSRowBytes)
	{
		CM_DecompressPVS(pModel->leafs[i].compressed_vis, scan, rows);

		if (i == 0)
		{
			continue;
		}

		for (j = 0; j < count; j++)
		{
			if (scan[j >> 3] & (1 << (j & 7)))
			{
				++vcount;
			}
		}
	}

	gPAS = (byte*)Mem_Calloc(gPVSRowBytes, count);

	acount = 0;
	scan = gPVS;
	dest = (unsigned int*)gPAS;
	for (i = 0; i < count; i++, scan += gPVSRowBytes, dest += rowwords)
	{
		Q_memcpy(dest, scan, gPVSRowBytes);

		for (j = 0; j < gPVSRowBytes; j++)
		{
			bitbyte = scan[j];
			if (bitbyte == 0)
			{
				continue;
			}

			for (k = 0; k < 8; k++)
			{
				if (!(bitbyte & (1 << k)))
				{
					continue;
				}

				index = j * 8 + k + 1;
				if (index >= count)
				{
					continue;
				}

				src = (unsigned int*)&gPVS[index * gPVSRowBytes];
				for (l = 0; l < rowwords; l++)
				{
					dest[l] |= src[l];
				}
			}
		}

		if (i == 0)
		{
			continue;
		}

		for (j = 0; j < count; j++)
		{
			if (((byte*)dest)[j >> 3] & (1 << (j & 7)))
			{
				++acount;
			}
		}
	}

	Con_DPrintf("Average leaves visible / audible / total: %i / %i / %i\n", vcount / count, acount / count, count);
}
