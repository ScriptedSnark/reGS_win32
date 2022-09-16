#include "quakedef.h"
#include "host.h"
#include "gl_draw.h"
#include "net_chan.h"
#include "client.h"
#include "ws2def.h"
#include <WinSock2.h>
#include <vid.h>

// As reHLDS says - this is the original statement.
#define INV_SOCK 0

#define MAX_LOOPBACK 4

qboolean net_thread_initialized;

int net_configured;
SOCKET ip_sockets[NS_MAX] = {INV_SOCK, INV_SOCK, INV_SOCK};
sizebuf_t net_message;
qboolean noip;
qboolean noipx;
qboolean use_thread;

unsigned char net_message_buffer[NET_MAX_PAYLOAD];
unsigned char in_message_buf[NET_MAX_PAYLOAD];
sizebuf_t in_message;

packetlag_t g_pLagData[NS_MAX];

HANDLE hNetThread;
DWORD dwNetThreadId;
CRITICAL_SECTION net_cs;

typedef int socklen_t;

typedef struct
{
	byte data[NET_MAX_MESSAGE];
	int datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t msgs[MAX_LOOPBACK];
	int get;
	int send;
} loopback_t;

typedef struct kbutton_s
{
	int down[2]; // key nums holding it down
	int state;	 // low bit is down state
} kbutton_t;

static loopback_t loopbacks[2];
int net_sleepforever = 1;
net_messages_t* normalqueue;

cvar_t net_address = {"net_address", "", 0, 0.0f, NULL};
cvar_t ipname = {"ip", "localhost", 0, 0.0f, NULL};
cvar_t defport = {"port", "27015", 0, 0.0f, NULL};
cvar_t ip_clientport = {"ip_clientport", "0", 0, 0.0f, NULL};
cvar_t clientport = {"clientport", "27005", 0, 0.0f, NULL};
cvar_t clockwindow = {"clockwindow", "0.5", 0, 0.0f, NULL};
cvar_t iphostport = {"ip_hostport", "0", 0, 0.0f, NULL};
cvar_t hostport = {"hostport", "0", 0, 0.0f, NULL};
cvar_t multicastport = {"multicastport", "27025", 0, 0.0f, NULL};
cvar_t ipx_hostport = {"ipx_hostport", "0", 0, 0.0f, NULL};
cvar_t ipx_clientport = {"ipx_clientport", "0", 0, 0.0f, NULL};
cvar_t fakelag = {"fakelag", "0.0", 0, 0.0f, NULL};
cvar_t fakeloss = {"fakeloss", "0.0", 0, 0.0f, NULL};
cvar_t net_graph = {"net_graph", "0", FCVAR_ARCHIVE, 0.0f, NULL};
cvar_t net_graphwidth = {"net_graphwidth", "150", 0, 0.0f, NULL};
cvar_t net_scale = {"net_scale", "5", FCVAR_ARCHIVE, 0.0f, NULL};
cvar_t net_graphpos = {"net_graphpos", "1", FCVAR_ARCHIVE, 0.0f, NULL};

void NET_ThreadLock()
{
	if (use_thread && net_thread_initialized)
	{
		EnterCriticalSection(&net_cs);
	}
}

void NET_ThreadUnlock()
{
	if (use_thread && net_thread_initialized)
	{
		LeaveCriticalSection(&net_cs);
	}
}

// https://github.com/dreamstalker/rehlds/blob/2f0a402f9d14f05463a3c1457694fe0d57a61012/rehlds/engine/net_ws.cpp#L126
void NetadrToSockadr(const netadr_t* a, struct sockaddr* s)
{
	Q_memset(s, 0, sizeof(*s));

	auto s_in = (sockaddr_in*)s;

	switch (a->type)
	{
	case NA_BROADCAST:
		s_in->sin_family = AF_INET;
		s_in->sin_addr.s_addr = INADDR_BROADCAST;
		s_in->sin_port = a->port;
		break;
	case NA_IP:
		s_in->sin_family = AF_INET;
		s_in->sin_addr.s_addr = *(int*)&a->ip;
		s_in->sin_port = a->port;
		break;
	default:
		break;
	}
}

qboolean NET_StringToSockaddr(const char* s, struct sockaddr* sadr)
{
	struct hostent* h;
	char* colon;
	char copy[128];

	Q_memset(sadr, 0, sizeof(*sadr));
	((struct sockaddr_in*)sadr)->sin_family = AF_INET;
	((struct sockaddr_in*)sadr)->sin_port = 0;

	Q_strcpy(copy, s);
	// strip off a trailing :port if present
	for (colon = copy; *colon; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			((struct sockaddr_in*)sadr)->sin_port = htons((short)atoi(colon + 1));
		}

	if (copy[0] >= '0' && copy[0] <= '9' && Q_strstr(copy, "."))
	{
		*(int*)&((struct sockaddr_in*)sadr)->sin_addr = inet_addr(copy);
	}
	else
	{
		if (!(h = gethostbyname(copy)))
			return 0;
		*(int*)&((struct sockaddr_in*)sadr)->sin_addr = *(int*)h->h_addr_list[0];
	}

	return true;
}


void NET_GetLocalAddress()
{
	memset(&net_local_adr, 0, 20);

	char* s;

	if (noip)
		Con_Printf("TCP/IP Disabled.\n");
	else
	{
		char buff[512];
		if (strcmp(ipname.string, "localhost"))
			strncpy(buff, ipname.string, sizeof(buff) - 1);
		else
			gethostname(buff, sizeof(buff));

		buff[511] = 0;

		sockaddr sadr;

		if (strcmp(buff, "localhost") && strcmp(buff, "127.0.0.1"))
		{
			if (NET_StringToSockaddr(buff, &sadr) && sadr.sa_family == 2)
			{
				net_local_adr.type = NA_IP;
				*(DWORD*)net_local_adr.ip = *(DWORD*)&sadr.sa_data[2];
				net_local_adr.port = (DWORD)sadr.sa_data;
			}
		}
		else
		{
			memset(&net_local_adr, 0, 20);
			net_local_adr.type = NA_LOOPBACK;
		}

		int ip_socket = ip_sockets[1];
		socklen_t namelen[4];
		namelen[0] = 16;

		sockaddr_in address;
		if (getsockname(ip_socket, (sockaddr*)&address, namelen))
		{
			int* error_location = _errno();
			noip = true;
			char* error_reason = strerror(*error_location);
			Con_Printf("Could not get TCP/IP address, TCP/IP disabled\nReason:  %s\n", error_reason);
		}
		else
		{
			netadrtype_t type = net_local_adr.type;

			net_local_adr.port = address.sin_port;

			if (type == NA_LOOPBACK)
				snprintf(s, 64, "loopback");
			else if (type == NA_IP)
			{
				snprintf(
					s,
					64,
					"%i.%i.%i.%i:%i",
					net_local_adr.ip[0],
					net_local_adr.ip[1],
					net_local_adr.ip[2],
					net_local_adr.ip[3],
					(net_local_adr.port >> 8 | net_local_adr.port << 8));
			}

			Con_Printf("Server IP address %s\n", s);

			char* net_adr = va((char*)"%s", s);
			Cvar_Set("net_address", net_adr);
		}
	}
}

void NET_Config(qboolean multiplayer)
{
	static qboolean old_config;

	if (old_config == multiplayer)
	{
		return;
	}

	old_config = multiplayer;

	if (multiplayer)
	{
		// if (!noip)
		// NET_OpenIP(); - TODO: implement - ScriptedSnark
		// if (!noipx)
		//  NET_OpenIPX(); - TODO: implement - ScriptedSnark

		static qboolean bFirst = TRUE;
		if (bFirst)
		{
			bFirst = FALSE;
			NET_GetLocalAddress();
		}
	}
	else
	{
		NET_ThreadLock();

		for (int sock = 0; sock < NS_MAX; sock++)
		{
			if (ip_sockets[sock] != INV_SOCK)
			{
				closesocket(ip_sockets[sock]);
				ip_sockets[sock] = INV_SOCK;
			}

			/* - TODO: implement IPX sockets - ScriptedSnark
			if (ipx_sockets[sock] != INV_SOCK)
			{
				closesocket(ipx_sockets[sock]);
				ipx_sockets[sock] = INV_SOCK;
			}
			*/
		}

		NET_ThreadUnlock();
	}

	net_configured = multiplayer ? 1 : 0;
}

void NET_RemoveFromPacketList(packetlag_t* pPacket)
{
	pPacket->pPrev->pNext = pPacket->pNext;
	pPacket->pNext->pPrev = pPacket->pPrev;
	pPacket->pPrev = 0;
	pPacket->pNext = 0;
}

void NET_StartThread()
{
	if (use_thread)
	{
		if (net_thread_initialized == false)
			net_thread_initialized = true;
	}
}

void NET_AllocateQueues()
{
	for (int i = 0; i < NUM_MSG_QUEUES; i++)
	{
		net_messages_t* p = (net_messages_t*)Mem_ZeroMalloc(sizeof(net_messages_t));
		p->buffer = (unsigned char*)Mem_ZeroMalloc(MSG_QUEUE_SIZE);
		p->preallocated = TRUE;
		p->next = normalqueue;
		normalqueue = p;
	}

	NET_StartThread();
}

void MaxPlayers_f()
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("\"maxplayers\" is \"%u\"\n", svs.maxclients);
		return;
	}

	if (sv.active)
	{
		Con_Printf("maxplayers cannot be changed while a server is running.\n");
		return;
	}


	int n = Q_atoi(Cmd_Argv(1));
	if (n < 1)
		n = 1;

	if (n > svs.maxclientslimit)
	{
		n = svs.maxclientslimit;
		Con_Printf("\"maxplayers\" set to \"%u\"\n", svs.maxclientslimit);
	}
	svs.maxclients = n;

	if (n == 1)
		Cvar_Set("deathmatch", "0");
	else
		Cvar_Set("deathmatch", "1");
}

void NET_Init()
{
	Cmd_AddCommand("maxplayers", MaxPlayers_f);

	Cvar_RegisterVariable(&net_address);
	Cvar_RegisterVariable(&ipname);
	Cvar_RegisterVariable(&iphostport);
	Cvar_RegisterVariable(&hostport);
	Cvar_RegisterVariable(&defport);
	Cvar_RegisterVariable(&ip_clientport);
	Cvar_RegisterVariable(&clientport);
	Cvar_RegisterVariable(&clockwindow);
	Cvar_RegisterVariable(&multicastport);
	Cvar_RegisterVariable(&ipx_hostport);
	Cvar_RegisterVariable(&ipx_clientport);
	Cvar_RegisterVariable(&fakelag);
	Cvar_RegisterVariable(&fakeloss);
	Cvar_RegisterVariable(&net_graph);
	Cvar_RegisterVariable(&net_graphwidth);
	Cvar_RegisterVariable(&net_scale);
	Cvar_RegisterVariable(&net_graphpos);

	if (COM_CheckParm("-netthread"))
		use_thread = TRUE;

	if (COM_CheckParm("-netsleep"))
		net_sleepforever = 0;

#ifdef _WIN32
	if (COM_CheckParm("-noipx"))
		noipx = TRUE;
#endif // _WIN32

	if (COM_CheckParm("-noip"))
		noip = TRUE;

	int port = COM_CheckParm("-port");
	if (port)
		Cvar_SetValue("hostport", Q_atof(com_argv[port + 1]));

	int clockwindow_ = COM_CheckParm("-clockwindow");
	if (clockwindow_)
		Cvar_SetValue("clockwindow", Q_atof(com_argv[clockwindow_ + 1]));

	net_message.data = (byte*)&net_message_buffer;
	net_message.maxsize = sizeof(net_message_buffer);
	net_message.flags = 0;
	net_message.buffername = "net_message";

	in_message.data = (byte*)&in_message_buf;
	in_message.maxsize = sizeof(in_message_buf);
	in_message.flags = 0;
	in_message.buffername = "in_message";

	for (int i = 0; i < NS_MAX; i++)
	{
		g_pLagData[i].pPrev = &g_pLagData[i];
		g_pLagData[i].pNext = &g_pLagData[i];
	}

	NET_AllocateQueues();
	Con_DPrintf("Base networking initialized.\n");
}

void NET_Shutdown()
{
	NET_ThreadLock();

	// NET_ClearLaggedList(g_pLagData); - TODO: implement - ScriptedSnark
	// NET_ClearLaggedList(&g_pLagData[1]); - TODO: implement - ScriptedSnark

	NET_ThreadUnlock();

	NET_Config(FALSE);
	// NET_FlushQueues(); - TODO: implement - ScriptedSnark
}

char* NET_ErrorString(int __errnum)
{
	return strerror(__errnum);
}

char* NET_AdrToString(netadr_t a)
{
	char* s;
	Q_memset(s, 0, 64);
	if (a.type == NA_LOOPBACK)
	{
		snprintf(s, 64, "loopback");
	}
	else if (a.type == NA_IP)
	{
		snprintf(s, 64, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2],
			a.ip[3], (a.port >> 8 | a.port << 8));
	}
	return s;
}

void SockadrToNetadr(sockaddr* s, netadr_t* a)
{
	if (s->sa_family != AF_INET)
	{
		return;
	}
	a->type = NA_IP;
	*(int*)&a->ip = ((struct sockaddr_in*)s)->sin_addr.s_addr;
	a->port = ((struct sockaddr_in*)s)->sin_port;
}

int NET_SendTo(bool verbose, SOCKET s, const char FAR* buf, int len, int flags, const struct sockaddr FAR* to, int tolen)
{
	return sendto(s, buf, len, flags, to, tolen);
}

void NET_SendLoopPacket(netsrc_t sock, int length, void* data, netadr_t to)

{
	loopback_t* loop = &loopbacks[sock ^ 1];

	int i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	// Thanks to reHLDS for this check.
	if (sizeof(loop->msgs[i].data) < length)
	{
		Sys_Error("%s: data size is bigger than message storage size", __func__);
	}

	Q_memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

int NET_SendLong(netsrc_t sock, SOCKET s, const char* buf, int len, const struct sockaddr* to, int tolen, int flags)
{
	static long gSequenceNumber = 1;

	if (sock == NS_SERVER && len > 1401)
	{
		gSequenceNumber = gSequenceNumber + 1;
		if (gSequenceNumber < 0)
		{
			gSequenceNumber = 1;
		}

		char packet[MAX_ROUTEABLE_PACKET];
		SPLITPACKET* pPacket = (SPLITPACKET*)packet;
		pPacket->netID = NET_HEADER_FLAG_SPLITPACKET;
		pPacket->sequenceNumber = gSequenceNumber;
		int packetNumber = 0;
		int totalSent = 0;
		int packetCount = (len + SPLIT_SIZE - 1) / SPLIT_SIZE;

		while (len > 0)
		{
			int size = Q_min(int(SPLIT_SIZE), len);

			pPacket->packetID = (packetNumber << 8) + packetCount;

			memcpy(packet + sizeof(SPLITPACKET), buf + (packetNumber * SPLIT_SIZE), size);

			int ret = NET_SendTo(false, s, packet, size + sizeof(SPLITPACKET), flags, to, tolen);
			if (ret < 0)
			{
				return ret;
			}

			if (ret >= size)
			{
				totalSent += size;
			}

			len -= size;
			packetNumber++;

			// This can be 15, but if you have a lot of packets, that will pause the server for a long time - Source Beta 2003
			Sleep(1);

			/*if (net_showpackets.GetInt())
			{
				netadr_t adr;
				memset(&adr, 0, sizeof(adr));

				SockadrToNetadr((struct sockaddr*)to, &adr);

				Con_Printf("Split packet %i/%i (size %i dest:  %s:%s)\n",
					packetNumber,
					packetCount,
					originalSize,
					NET_GetPlayerNameForAdr(&adr),
					NET_AdrToString(adr));
			}*/
		}

		return totalSent;
	}
	else
	{
		int nSend = 0;
		nSend = NET_SendTo(true, s, buf, len, flags, to, tolen);
		return nSend;
	}
}

void NET_SendPacket(netsrc_t sock, int length, void* data, netadr_t to)
{
	SOCKET net_socket;
	if (to.type == NA_LOOPBACK)
	{
		NET_SendLoopPacket(sock, length, data, to);
		return;
	}
	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_sockets[sock];
		if (net_socket == INV_SOCK) // Can be also like "if (!net_socket)"
			return;
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_sockets[sock];
		if (net_socket == INV_SOCK) // Can be also like "if (!net_socket)"
			return;
	}
	// Where is IPX type? - xWhitey
	else
	{
		Sys_Error("NET_SendPacket: bad address type");
		return;
	}

	struct sockaddr addr;
	NetadrToSockadr(&to, &addr);

	// NET_SendLong(netsrc_t sock, SOCKET s, const char* buf, int len, const struct sockaddr* to, int tolen, int flags)
	// reHLDS has flags param before sockaddr ptr named to
	int ret = NET_SendLong(sock, net_socket, (const char*)data, length, &addr, sizeof(addr), 0);
	if (ret == -1)
	{
		int err;

		err = GetLastError();

		if (err == WSAEWOULDBLOCK)
			return;

		if (err == WSAECONNRESET)
			return;

		if ((err == WSAEADDRNOTAVAIL) && (to.type == NA_BROADCAST))
			return;

		if (cls.state == ca_dedicated)
		{
			Con_Printf("NET_SendPacket ERROR: %s\n", NET_ErrorString(err));
		}
		else
		{
			if (err == WSAEADDRNOTAVAIL)
			{
				Con_DPrintf("NET_SendPacket Warning: %s : %s\n", NET_ErrorString(err), NET_AdrToString(to));
			}
			else
			{
				Sys_Error("NET_SendPacket ERROR: %s\n", NET_ErrorString(err));
			}
		}
	}
}

void NET_DrawString(int x, int y, int font, float r, float g, float b, char* fmt, ...)
{
	static char string[1024];
	va_list varargs;

	va_start(varargs, fmt);
	vsnprintf(string, sizeof(string), fmt, varargs);
	va_end(varargs);

	Draw_SetTextColor(r, g, b);
	Draw_String(x, y, string);
}

/*
==================
SCR_NetGraph

Visualizes data flow
==================
*/
void SCR_NetGraph()
{
	// impl - xWhitey
	/*int value = net_graph.value;
	int incoming_sequence, outgoing_sequence, height, width;
	kbutton_s* key = ClientDLL_FindKey("in_graph");
	if (!value) {
		if (!key || (key->state & 1) == 0) return;
		value = 2;
	}
	if (net_scale.value <= 0.0) net_scale.value = 1.0;
	incoming_sequence = cls.netchan.incoming_sequence;
	height = vid.height;
	width = vid.width;*/
}

static double rolling_fps;

void SCR_DrawFPS(void)
{
	if (!cl_showfps.value)
		return;

	if (host_frametime <= 0.0)
		return;

	rolling_fps = 0.6 * rolling_fps + host_frametime * 0.4;
	NET_DrawString(2, 2, 0, 1, 1, 1, "%d fps", (int)floor(1.0 / rolling_fps));
}
