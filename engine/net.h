/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef ENGINE_NET_H
#define ENGINE_NET_H

#include <cstdlib>
#include "tier0/platform.h"
#include "netadr.h"

// Draw client info
void SCR_DrawFPS(void);
void SCR_NetGraph(void);

/**
 *	Maximum size for a fragment buffer.
 */
// TODO: unrelated to actual net constants, remove - Solokiller
const size_t NET_MAX_FRAG_BUFFER = 1400;

#pragma pack(push, 1)
typedef struct SPLITPACKET_t
{
	int netID;
	int sequenceNumber;
	unsigned char packetID;
} SPLITPACKET;
#pragma pack(pop)

#define MAX_MSGLEN 4000	  // max length of a reliable message
#define MAX_DATAGRAM 4000 // max length of unreliable message

#define MAX_ROUTEABLE_PACKET 1400 // probably should be 0x579 (1401 in decimal)
#define SPLIT_SIZE (MAX_ROUTEABLE_PACKET - sizeof(SPLITPACKET))

#define NET_HEADER_FLAG_QUERY -1
#define NET_HEADER_FLAG_SPLITPACKET -2
#define NET_HEADER_FLAG_COMPRESSEDPACKET -3

#define MAX_STREAMS 2

#define HEADER_BYTES (8 + MAX_STREAMS * 9)

#define NET_MAX_PAYLOAD 96000

#define NET_MAX_MESSAGE PAD_NUMBER((NET_MAX_PAYLOAD + HEADER_BYTES), 16)

extern sizebuf_t net_message;

const int FRAGMENT_MAX_SIZE = 1024;

extern cvar_t net_address;
extern cvar_t ipname;
extern cvar_t defport;
extern cvar_t ip_clientport;
extern cvar_t clientport;
extern int net_sleepforever;
extern float gFakeLag;
extern int net_configured;
extern netadr_t net_local_ipx_adr;
extern netadr_t net_local_adr;
extern netadr_t net_from;
extern qboolean noip;
extern qboolean noipx;
extern sizebuf_t net_message;
extern cvar_t clockwindow;
extern qboolean use_thread;
extern cvar_t iphostport;
extern cvar_t hostport;
extern cvar_t ipx_hostport;
extern cvar_t ipx_clientport;
extern cvar_t multicastport;
extern cvar_t fakelag;
extern cvar_t fakeloss;
extern cvar_t net_graph;
extern cvar_t net_graphwidth;
extern cvar_t net_scale;
extern cvar_t net_graphpos;
extern unsigned char net_message_buffer[NET_MAX_PAYLOAD];
extern unsigned char in_message_buf[NET_MAX_PAYLOAD];

const char A2A_PRINT = 'l';

typedef enum svc_commands_e
{
	svc_bad,
	svc_nop,
	svc_disconnect,
	svc_event,
	svc_version,
	svc_setview,
	svc_sound,
	svc_time,
	svc_print,
	svc_stufftext,
	svc_setangle,
	svc_serverinfo,
	svc_lightstyle,
	svc_updateuserinfo,
	svc_deltadescription,
	svc_clientdata,
	svc_stopsound,
	svc_pings,
	svc_particle,
	svc_damage,
	svc_spawnstatic,
	svc_event_reliable,
	svc_spawnbaseline,
	svc_temp_entity,
	svc_setpause,
	svc_signonnum,
	svc_centerprint,
	svc_killedmonster,
	svc_foundsecret,
	svc_spawnstaticsound,
	svc_intermission,
	svc_finale,
	svc_cdtrack,
	svc_restore,
	svc_cutscene,
	svc_weaponanim,
	svc_decalname,
	svc_roomtype,
	svc_addangle,
	svc_newusermsg,
	svc_packetentities,
	svc_deltapacketentities,
	svc_choke,
	svc_resourcelist,
	svc_newmovevars,
	svc_resourcerequest,
	svc_customization,
	svc_crosshairangle,
	svc_soundfade,
	svc_filetxferfailed,
	svc_hltv,
	svc_director,
	svc_voiceinit,
	svc_voicedata,
	svc_sendextrainfo,
	svc_timescale,
	svc_resourcelocation,
	svc_sendcvarvalue,
	svc_sendcvarvalue2,
	svc_exec,
	svc_reserve60,
	svc_reserve61,
	svc_reserve62,
	svc_reserve63,
	// Let's just use an id of the first user message instead of the last svc_*
	// This change makes code in `PF_MessageEnd_I` forward-compatible with future svc_* additions
	svc_startofusermessages = svc_exec,
	svc_endoflist = 255,
} svc_commands_t;

typedef struct net_messages_s
{
	net_messages_s* next;
	qboolean preallocated;
	unsigned char* buffer;
	netadr_t from;
	int buffersize;
} net_messages_t;

typedef struct packetlag_s
{
	unsigned char* pPacketData;
	int nSize;
	netadr_t net_from_;
	float receivedTime;
	struct packetlag_s* pNext;
	struct packetlag_s* pPrev;
} packetlag_t;

typedef struct flowstats_s
{
	// Size of message sent/received
	int size;
	// Time that message was sent/received
	double time;
} flowstats_t;

const int MAX_LATENT = 32;

const int NUM_MSG_QUEUES = 40;
const int MSG_QUEUE_SIZE = 1536;

typedef struct flow_s
{
	// Data for last MAX_LATENT messages
	flowstats_t stats[MAX_LATENT];
	// Current message position
	int current;
	// Time when we should recompute k/sec data
	double nextcompute;
	// Average data
	float kbytespersec;
	float avgkbytespersec;
} flow_t;

typedef struct fragbuf_s
{
	fragbuf_s* next;
	int bufferid;
	sizebuf_t frag_message;
	byte frag_message_buf[FRAGMENT_MAX_SIZE];
	qboolean isfile;
	qboolean isbuffer;
	qboolean iscompressed;
	char filename[MAX_PATH];
	int foffset;
	int size;
} fragbuf_t;

typedef struct fragbufwaiting_s
{
	fragbufwaiting_s* next;
	int fragbufcount;
	fragbuf_t* fragbufs;
} fragbufwaiting_t;

extern packetlag_t g_pLagData[3];
extern net_messages_t* normalqueue;

void NET_ThreadLock();
void NET_ThreadUnlock();
void NET_Config(qboolean multiplayer);

void NET_Shutdown();

void NET_StartThread();

void NET_AllocateQueues();

void NET_Init();

char* NET_AdrToString(netadr_t a);
qboolean NET_StringToSockaddr(const char* s, struct sockaddr* sadr);

#endif // ENGINE_NET_H
