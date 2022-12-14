#ifndef ENGINE_LOADBLOB_H
#define ENGINE_LOADBLOB_H

#include <tier0/platform.h>

struct BlobFootprint_t
{
	int32 m_hDll;
};

using FFunction = void (*)(void*);

int FIsBlob(const char* pstFileName);

void FreeBlob(BlobFootprint_t* pblobfootprint);

void* NLoadBlobFile(const char* pstFileName, BlobFootprint_t* pblobfootprint, void* pv, char fLoadAsDll);

#endif // ENGINE_LOADBLOB_H
