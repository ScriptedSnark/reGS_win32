#ifndef ENGINE_MODEL_H
#define ENGINE_MODEL_H

extern unsigned char* gPAS;
extern unsigned char* gPVS;
const int MODEL_MAX_PVS = 1024;

void Mod_Init();
byte* Mod_DecompressVis(byte* in, model_t* model);
unsigned char* Mod_LeafPVS(mleaf_t* leaf, model_t* model);
unsigned char* CM_LeafPVS(int leafnum);
unsigned char* CM_LeafPAS(int leafnum);
void CM_FreePAS();
void CM_CalcPAS(model_t* pModel);

bool IsCZPlayerModel(uint32 crc, const char* filename);

#endif // ENGINE_MODEL_H
