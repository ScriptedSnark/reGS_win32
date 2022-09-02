#ifndef ENGINE_HASHPAK_H
#define ENGINE_HASHPAK_H

#include "custom.h"

#define HASHPAK_EXTENSION ".hpk"
#define HASHPAK_VERSION 0x0001

#define MAX_FILE_SIZE 0x20000
#define MAX_FILE_ENTRIES 0x8000

typedef struct hash_pack_entry_s
{
	resource_t resource;
	int nOffset;
	int nFileLength;
} hash_pack_entry_t;

typedef struct hash_pack_directory_s
{
	int nEntries;
	hash_pack_entry_t* p_rgEntries;
} hash_pack_directory_t;

typedef struct hash_pack_header_s
{
	char szFileStamp[4];
	int version;
	int nDirectoryOffset;
} hash_pack_header_t;

typedef struct hash_pack_queue_s
{
	char* pakname;
	resource_t resource;
	int datasize;
	void* data;
	hash_pack_queue_s* next;
} hash_pack_queue_t;

extern hash_pack_queue_t* gp_hpak_queue;

void HPAK_FlushHostQueue();

void HPAK_Init();

void HPAK_ValidatePak(char* fullpakname);
void HPAK_CheckIntegrity(char* pakname);

void HPAK_AddLump(bool bUseQueue, const char* pakname, resource_t* pResource, void* pData, FileHandle_t fpSource);
qboolean HPAK_FindResource(hash_pack_directory_t* pDir, unsigned char* hash, struct resource_s* pResourceEntry);
qboolean HPAK_ResourceForHash(char* pakname, unsigned char* hash, struct resource_s* pResourceEntry);

#endif // ENGINE_HASHPAK_H
