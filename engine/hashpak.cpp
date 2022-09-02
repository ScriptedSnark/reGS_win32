#include "quakedef.h"
#include "hashpak.h"

hash_pack_queue_t* gp_hpak_queue = NULL;

void HPAK_FlushHostQueue()
{
	for (hash_pack_queue_t* p = gp_hpak_queue; gp_hpak_queue != NULL; p = gp_hpak_queue)
	{
		gp_hpak_queue = p->next;
		HPAK_AddLump(0, p->pakname, &p->resource, p->data, 0);
		Mem_Free(p->pakname);
		Mem_Free(p->data);
		Mem_Free(p);
	}
}

void HPAK_AddLump(bool bUseQueue, const char* pakname, resource_t* pResource, void* pData, FileHandle_t fpSource)
{
	// TODO: implement - Solokiller
}

void HPAK_Init()
{
	/*
	Cmd_AddCommand("hpklist", HPAK_List_f);
	Cmd_AddCommand("hpkremove", HPAK_Remove_f);
	Cmd_AddCommand("hpkval", HPAK_Validate_f);
	Cmd_AddCommand("hpkextract", HPAK_Extract_f);
	gp_hpak_queue = false;
	*/
}

void HPAK_ValidatePak(char* fullpakname)
{
	hash_pack_header_t header;
	hash_pack_directory_t directory;
	hash_pack_entry_t* entry;
	char szFileName[MAX_PATH];
	FileHandle_t fp;
	byte* pData;
	byte md5[16];
	MD5Context_t ctx;

	HPAK_FlushHostQueue();
	fp = FS_Open(fullpakname, "rb");

	if (!fp)
		return;

	FS_Read(&header, sizeof(hash_pack_header_t), fp);

	if (header.version != HASHPAK_VERSION || Q_strncmp(header.szFileStamp, "HPAK", sizeof(header.szFileStamp)) != 0)
	{
		Con_Printf("%s is not a PAK file, deleting\n", fullpakname);
		FS_Close(fp);
		FS_RemoveFile(fullpakname, 0);
		return;
	}

	FS_Seek(fp, header.nDirectoryOffset, FILESYSTEM_SEEK_HEAD);
	FS_Read(&directory, 4, fp);

	if (directory.nEntries < 1 || (unsigned int)directory.nEntries > MAX_FILE_ENTRIES)
	{
		Con_Printf("ERROR: HPAK %s had bogus # of directory entries:  %i, deleting\n", fullpakname, directory.nEntries);
		FS_Close(fp);
		FS_RemoveFile(fullpakname, 0);
		return;
	}

	directory.p_rgEntries = (hash_pack_entry_t*)Mem_Malloc(sizeof(hash_pack_entry_t) * directory.nEntries);
	FS_Read(directory.p_rgEntries, sizeof(hash_pack_entry_t) * directory.nEntries, fp);

	for (int nCurrent = 0; nCurrent < directory.nEntries; nCurrent++)
	{
		entry = &directory.p_rgEntries[nCurrent];
		COM_FileBase(entry->resource.szFileName, szFileName);

		if ((unsigned int)entry->nFileLength >= MAX_FILE_SIZE)
		{
			Con_Printf("Mismatched data in HPAK file %s, deleting\n", fullpakname);
			Con_Printf("Unable to MD5 hash data lump %i, size invalid:  %i\n", nCurrent + 1, entry->nFileLength);

			FS_Close(fp);
			FS_RemoveFile(fullpakname, 0);
			Mem_Free(directory.p_rgEntries);
			return;
		}

		pData = (byte*)Mem_Malloc(entry->nFileLength + 1);

		Q_memset(pData, 0, entry->nFileLength);
		FS_Seek(fp, entry->nOffset, FILESYSTEM_SEEK_HEAD);
		FS_Read(pData, entry->nFileLength, fp);
		Q_memset(&ctx, 0, sizeof(MD5Context_t));

		MD5Init(&ctx);
		MD5Update(&ctx, pData, entry->nFileLength);
		MD5Final(md5, &ctx);

		if (pData)
			Mem_Free(pData);

		if (Q_memcmp(entry->resource.rgucMD5_hash, md5, sizeof(md5)) != 0)
		{
			Con_Printf("Mismatched data in HPAK file %s, deleting\n", fullpakname);
			FS_Close(fp);
			FS_RemoveFile(fullpakname, 0);
			Mem_Free(directory.p_rgEntries);
			return;
		}
	}

	FS_Close(fp);
	Mem_Free(directory.p_rgEntries);
}

void HPAK_CheckIntegrity(char* pakname)
{
	char name[256];
	Q_snprintf(name, sizeof(name), "%s", pakname);

	COM_DefaultExtension(name, HASHPAK_EXTENSION);
	COM_FixSlashes(name);
	HPAK_ValidatePak(name);
}

qboolean HPAK_FindResource(hash_pack_directory_t* pDir, unsigned char* hash, struct resource_s* pResourceEntry)
{
	for (int i = 0; i < pDir->nEntries; i++)
	{
		if (Q_memcmp(hash, pDir->p_rgEntries[i].resource.rgucMD5_hash, 16) == 0)
		{
			if (pResourceEntry)
				Q_memcpy(pResourceEntry, &pDir->p_rgEntries[i].resource, sizeof(resource_t));

			return TRUE;
		}
	}
	return FALSE;
}

qboolean HPAK_ResourceForHash(char* pakname, unsigned char* hash, struct resource_s* pResourceEntry)
{
	qboolean bFound;
	hash_pack_header_t header;
	hash_pack_directory_t directory;
	char name[MAX_PATH];
	FileHandle_t fp;

	if (gp_hpak_queue)
	{
		for (hash_pack_queue_t* p = gp_hpak_queue; p != NULL; p = p->next)
		{
			if (Q_stricmp(p->pakname, pakname) != 0 || Q_memcmp(p->resource.rgucMD5_hash, hash, 16) != 0)
				continue;

			if (pResourceEntry)
				Q_memcpy(pResourceEntry, &p->resource, sizeof(resource_t));

			return TRUE;
		}
	}

	Q_snprintf(name, ARRAYSIZE(name), "%s", pakname);

	COM_DefaultExtension(name, HASHPAK_EXTENSION);
	fp = FS_Open(name, "rb");

	if (!fp)
	{
		Con_Printf("ERROR: couldn't open %s.\n", name);
		return FALSE;
	}

	FS_Read(&header, sizeof(hash_pack_header_t), fp);

	if (Q_strncmp(header.szFileStamp, "HPAK", sizeof(header.szFileStamp)))
	{
		Con_Printf("%s is not an HPAK file\n", name);
		FS_Close(fp);
		return FALSE;
	}

	if (header.version != HASHPAK_VERSION)
	{
		Con_Printf("HPAK_List:  version mismatch\n");
		FS_Close(fp);
		return FALSE;
	}

	FS_Seek(fp, header.nDirectoryOffset, FILESYSTEM_SEEK_HEAD);
	FS_Read(&directory.nEntries, 4, fp);

	if (directory.nEntries < 1 || (unsigned int)directory.nEntries > MAX_FILE_ENTRIES)
	{
		Con_Printf("ERROR: HPAK had bogus # of directory entries:  %i\n", directory.nEntries);
		FS_Close(fp);
		return FALSE;
	}

	directory.p_rgEntries = (hash_pack_entry_t*)Mem_Malloc(sizeof(hash_pack_entry_t) * directory.nEntries);
	FS_Read(directory.p_rgEntries, sizeof(hash_pack_entry_t) * directory.nEntries, fp);

	bFound = HPAK_FindResource(&directory, hash, pResourceEntry);

	FS_Close(fp);
	Mem_Free(directory.p_rgEntries);

	return bFound;
}
