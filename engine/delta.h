#ifndef ENGINE_DELTA_H
#define ENGINE_DELTA_H

typedef struct delta_s delta_t;
typedef void (*encoder_t)(delta_t*, const unsigned char*, const unsigned char*);

typedef struct delta_stats_s
{
	int sendcount;
	int receivedcount;
} delta_stats_t;

typedef struct delta_description_s
{
	int fieldType;
	char fieldName[32];
	int fieldOffset;
	short int fieldSize;
	int significant_bits;
	float premultiply;
	float postmultiply;
	short int flags;
	delta_stats_t stats;
} delta_description_t;

union delta_marked_mask_t
{
	uint8 u8[8];
	uint32 u32[2];
	uint64 u64;
};

typedef struct delta_s
{
	int dynamic;
	int fieldCount;
	char conditionalencodename[32];
	encoder_t conditionalencode;
	delta_description_t* pdd;
} delta_t;

typedef struct delta_encoder_s delta_encoder_t;

struct delta_encoder_s
{
	delta_encoder_t* next;
	char* name;
	encoder_t conditionalencode;
};



typedef struct delta_link_s delta_link_t;
typedef struct delta_definition_s delta_definition_t;
typedef struct delta_definition_list_s delta_definition_list_t;
typedef struct delta_registry_s delta_registry_t;
typedef struct delta_info_s delta_info_t;

typedef struct delta_info_s
{
	delta_info_s* next;
	char* name;
	char* loadfile;
	delta_t* delta;
} delta_info_t;

void DELTA_FreeDescription(delta_t** ppdesc);
void DELTA_ClearEncoders(void);
encoder_t DELTA_LookupEncoder(char* name);
void DELTA_ClearRegistrations(void);
void DELTA_ClearDefinitions(void);
void DELTA_ClearStats(delta_t* p);
void DELTA_ClearStats_f(void);
void DELTA_PrintStats(const char* name, delta_t* p);
void DELTA_DumpStats_f(void);
void DELTA_Init(void);
void DELTA_Shutdown(void);

#endif // ENGINE_DELTA_H
