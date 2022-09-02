#ifndef ENGINE_GL_MODEL_H
#define ENGINE_GL_MODEL_H

#include "modelgen.h"
#include "spritegn.h"
#include "bspfile.h"

#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2
#define NL_CLIENT 3

#define MAX_MOD_KNOWN 1024

extern model_t mod_known[MAX_MOD_KNOWN];
extern int mod_numknown;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
struct mspriteframe_t
{
	int width;
	int height;
	float up, down, left, right;
	int gl_texturenum;
};

struct mspritegroup_t
{
	int numframes;
	float* intervals;
	mspriteframe_t* frames[1];
};

struct mspriteframedesc_t
{
	spriteframetype_t type;
	mspriteframe_t* frameptr;
};

struct msprite_t
{
	short type;
	short texFormat;
	int maxwidth;
	int maxheight;
	int numframes;
	int paloffset;
	float beamlength; // remove?
	void* cachespot;  // remove?
	mspriteframedesc_t frames[1];
};

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct
{
	int firstpose;
	int numposes;
	float interval;
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
	char name[16];
} maliasframedesc_t;

typedef struct
{
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
} maliasgroupframedesc_t;

typedef struct
{
	int numframes;
	int intervals;
	maliasgroupframedesc_t frames[1];
} maliasgroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s
{
	int facesfront;
	int vertindex[3];
} mtriangle_t;


#define MAX_SKINS 32
typedef struct
{
	int ident;
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;

	int numposes;
	int poseverts;
	int posedata; // numposes*poseverts trivert_t
	int commands; // gl command list with embedded s/t
	int gl_texturenum[MAX_SKINS][4];
	int texels[MAX_SKINS];		 // only for player skins
	maliasframedesc_t frames[1]; // variable sized
} aliashdr_t;

#define MAXALIASVERTS 1024
#define MAXALIASFRAMES 256
#define MAXALIASTRIS 2048

#define SURF_PLANEBACK 2
#define SURF_DRAWSKY 4
#define SURF_DRAWSPRITE 8
#define SURF_DRAWTURB 0x10
#define SURF_DRAWTILED 0x20
#define SURF_DRAWBACKGROUND 0x40
#define SURF_UNDERWATER 0x80
#define SURF_DONTWARP 0x100

extern aliashdr_t* pheader;
extern stvert_t stverts[MAXALIASVERTS];
extern mtriangle_t triangles[MAXALIASTRIS];
extern trivertx_t* poseverts[MAXALIASFRAMES];

void* Mod_Extradata(model_t* mod);
void Mod_ClearAll();
void Mod_FillInCRCInfo(bool trackCRC, int model_number);
model_t* Mod_FindName(bool trackCRC, const char* name);
bool Mod_ValidateCRC(const char* name, CRC32_t crc);
void Mod_NeedCRC(const char* name, bool needCRC);
void Mod_ChangeGame(void);
model_t* Mod_LoadModel(model_t* mod, const bool crash, const bool trackCRC);
model_t* Mod_ForName(const char* name, bool crash, bool trackCRC);
void Mod_MarkClient(model_t* pModel);
void Mod_LoadEntities(lump_t* l);
void Mod_LoadLighting(lump_t* l);
void Mod_LoadVisibility(lump_t* l);
void Mod_LoadVertexes(lump_t* l);
void Mod_LoadSubmodels(lump_t* l);
void Mod_LoadEdges(lump_t* l);
void CalcSurfaceExtents(msurface_t* s);
void Mod_SetParent(mnode_t* node, mnode_t* parent);
void Mod_LoadNodes(lump_t* l);
void Mod_LoadLeafs(lump_t* l);
void Mod_LoadClipnodes(lump_t* l);
void Mod_MakeHull0(void);
void Mod_LoadMarksurfaces(lump_t* l);
void Mod_LoadSurfedges(lump_t* l);
void Mod_LoadPlanes(lump_t* l);
float RadiusFromBounds(vec_t* mins, vec_t* maxs);
void Mod_LoadBrushModel(model_t* mod, void* buffer);
void Mod_SpriteTextureName(char* pszName, int nNameSize, const char* pcszModelName, int framenum);
void Mod_UnloadSpriteTextures(model_t* pModel);
void Mod_Print(void);

#endif // ENGINE_GL_MODEL_H
