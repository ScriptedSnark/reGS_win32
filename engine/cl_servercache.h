/*
 * reGS: reverse-engineered GoldSrc engine
 * File: cl_servercache.h
 */

#pragma once

#include <netadr.h>
#include <tier0/basetypes.h>

struct server_cache_t
{
	int inuse;
	netadr_t adr;
	netadr_t spec_adr;
	char name[256];
	char spec_name[256];
	char map[256];
	char desc[256];
	int players;
	int maxplayers;
	int bots;
	int has_spectator_address;
	char os;
	char type;
	qboolean is_private;
	qboolean is_vac;
};
