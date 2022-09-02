#include "quakedef.h"
#include "net_chan.h"

char gDownloadFile[256] = {};

cvar_t net_log = {"net_log", "0", 0, 0.0f, nullptr};
cvar_t net_showpackets = {"net_showpackets", "0", 0, 0.0f, nullptr};
cvar_t net_showdrop = {"net_showdrop", "0", 0, 0.0f, nullptr};
cvar_t net_drawslider = {"net_drawslider", "0", 0, 0.0f, nullptr};
cvar_t net_chokeloopback = {"net_chokeloop", "0", 0, 0.0f, nullptr};
cvar_t sv_filetransfercompression = {"sv_filetransfercompression", "1", 0, 0.0f, nullptr};
cvar_t sv_filetransfermaxsize = {"sv_filetransfermaxsize", "10485760", 0, 0.0f, nullptr};

void Netchan_Init()
{
	Cvar_RegisterVariable(&net_log);
	Cvar_RegisterVariable(&net_showpackets);
	Cvar_RegisterVariable(&net_showdrop);
	Cvar_RegisterVariable(&net_chokeloopback);
	Cvar_RegisterVariable(&net_drawslider);
	Cvar_RegisterVariable(&sv_filetransfercompression);
	Cvar_RegisterVariable(&sv_filetransfermaxsize);
}

void Netchan_Transmit(netchan_t* chan, int length, byte* data)
{
	// TODO: implement - Solokiller
}
