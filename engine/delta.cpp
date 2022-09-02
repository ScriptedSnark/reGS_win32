#include "quakedef.h"
#include "delta.h"

#define DELTA_D_DEF(member) #member, offsetof(delta_description_s, member)
#define DELTA_DEF(structname, member)         \
	{                                         \
#member, offsetof(structname, member) \
	}

typedef struct event_args_s
{
	int flags;

	int entindex;

	float origin[3];
	float angles[3];
	float velocity[3];

	int ducking;

	float fparam1;
	float fparam2;

	int iparam1;
	int iparam2;

	int bparam1;
	int bparam2;
} event_args_t;

typedef struct delta_link_s
{
	delta_link_t* next;
	delta_description_t* delta;
} delta_link_t;

typedef struct delta_definition_s
{
	char* fieldName;
	size_t fieldOffset;
} delta_definition_t;

typedef struct delta_definition_list_s
{
	delta_definition_list_t* next;
	char* ptypename;
	int numelements;
	delta_definition_t* pdefinition;
} delta_definition_list_t;

typedef struct delta_registry_s
{
	delta_registry_t* next;
	char* name;
	delta_t* pdesc;
} delta_registry_t;

delta_definition_list_t* g_defs;
delta_encoder_t* g_encoders;
delta_registry_t* g_deltaregistry;

static delta_definition_t g_EventDataDefinition[] =
	{
		DELTA_DEF(event_args_s, entindex),
		DELTA_DEF(event_args_s, origin[0]),
		DELTA_DEF(event_args_s, origin[1]),
		DELTA_DEF(event_args_s, origin[2]),
		DELTA_DEF(event_args_s, angles[0]),
		DELTA_DEF(event_args_s, angles[1]),
		DELTA_DEF(event_args_s, angles[2]),
		DELTA_DEF(event_args_s, fparam1),
		DELTA_DEF(event_args_s, fparam2),
		DELTA_DEF(event_args_s, iparam1),
		DELTA_DEF(event_args_s, iparam2),
		DELTA_DEF(event_args_s, bparam1),
		DELTA_DEF(event_args_s, bparam2),
		DELTA_DEF(event_args_s, ducking),
};

void DELTA_AddDefinition(char* name, delta_definition_t* pdef, int numelements)
{
	delta_definition_list_t* p = g_defs;
	while (p)
	{
		if (!Q_stricmp(name, p->ptypename))
		{
			break;
		}
		p = p->next;
	}

	if (p == NULL)
	{
		p = (delta_definition_list_t*)Mem_ZeroMalloc(sizeof(delta_definition_list_t));
		p->ptypename = Mem_Strdup(name);
		p->next = g_defs;
		g_defs = p;
	}

	p->pdefinition = pdef;
	p->numelements = numelements;
}

void DELTA_FreeDescription(delta_t** ppdesc)
{
	delta_t* p;

	if (ppdesc)
	{
		p = *ppdesc;
		if (p)
		{
			if (p->dynamic)
				Mem_Free(p->pdd);
			Mem_Free(p);
			*ppdesc = 0;
		}
	}
}

void DELTA_ClearEncoders(void)
{
	delta_encoder_t *n, *p = g_encoders;
	while (p)
	{
		n = p->next;
		Mem_Free(p->name);
		Mem_Free(p);
		p = n;
	}
	g_encoders = 0;
}

encoder_t DELTA_LookupEncoder(char* name)
{
	delta_encoder_t* p = g_encoders;
	while (p)
	{
		if (!Q_stricmp(name, p->name))
		{
			return p->conditionalencode;
		}
		p = p->next;
	}
	return NULL;
}

void DELTA_ClearDefinitions(void)
{
	delta_definition_list_t *n, *p = g_defs;
	while (p)
	{
		n = p->next;
		Mem_Free(p->ptypename);
		Mem_Free(p);
		p = n;
	}
	g_defs = 0;
}

void DELTA_ClearRegistrations(void)
{
	delta_registry_t *n, *p = g_deltaregistry;
	while (p)
	{
		n = p->next;
		Mem_Free(p->name);
		if (p->pdesc)
			DELTA_FreeDescription(&p->pdesc);
		Mem_Free(p);
		p = n;
	}
	g_deltaregistry = 0;
}

void DELTA_ClearStats(delta_t* p)
{
	int i;

	if (p)
	{
		for (i = p->fieldCount - 1; i >= 0; i--)
		{
			p->pdd[i].stats.sendcount = 0;
			p->pdd[i].stats.receivedcount = 0;
		}
	}
}

void DELTA_ClearStats_f(void)
{
	delta_registry_t* p;

	Con_Printf("Clearing delta stats\n");
	for (p = g_deltaregistry; p; p = p->next)
	{
		DELTA_ClearStats(p->pdesc);
	}
}

void DELTA_PrintStats(const char* name, delta_t* p)
{
	if (p)
	{
		Con_Printf("Stats for '%s'\n", name);
		if (p->fieldCount > 0)
		{
			delta_description_t* dt = p->pdd;
			for (int i = 0; i < p->fieldCount; i++, dt++)
			{
				Con_Printf("  %02i % 10s:  s % 5i r % 5i\n", i + 1, dt->fieldName, dt->stats.sendcount, dt->stats.receivedcount);
			}
		}
		Con_Printf("\n");
	}
}

void DELTA_DumpStats_f(void)
{
	Con_Printf("Delta Stats\n");
	for (delta_registry_t* dr = g_deltaregistry; dr; dr = dr->next)
		DELTA_PrintStats(dr->name, dr->pdesc);
}

void DELTA_Init(void)
{
	Cmd_AddCommand("delta_stats", DELTA_DumpStats_f);
	Cmd_AddCommand("delta_clear", DELTA_ClearStats_f);

	/* TODO: implement - ScriptedSnark
	DELTA_AddDefinition("clientdata_t", g_ClientDataDefinition, ARRAYSIZE(g_ClientDataDefinition));
	DELTA_AddDefinition("weapon_data_t", g_WeaponDataDefinition, ARRAYSIZE(g_WeaponDataDefinition));
	DELTA_AddDefinition("usercmd_t", g_UsercmdDataDefinition, ARRAYSIZE(g_UsercmdDataDefinition));
	DELTA_AddDefinition("entity_state_t", g_EntityDataDefinition, ARRAYSIZE(g_EntityDataDefinition));
	DELTA_AddDefinition("entity_state_player_t", g_EntityDataDefinition, ARRAYSIZE(g_EntityDataDefinition));
	DELTA_AddDefinition("custom_entity_state_t", g_EntityDataDefinition, ARRAYSIZE(g_EntityDataDefinition));
	*/
	DELTA_AddDefinition("event_t", g_EventDataDefinition, ARRAYSIZE(g_EventDataDefinition));
}

void DELTA_Shutdown(void)
{
	DELTA_ClearEncoders();
	DELTA_ClearDefinitions();
	DELTA_ClearRegistrations();
	// TODO: add registration, definition, encoder funcs - ScriptedSnark
}
