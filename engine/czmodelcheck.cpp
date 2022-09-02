/*
 * reGS: reverse-engineered GoldSrc engine
 * File: czmodelcheck.cpp
 */

#include "quakedef.h"
#include "tier1/strtools.h"

bool IsCZPlayerModel(uint32 crc, const char* filename)
{
	if (crc == 0x27FB4D2F)
		return Q_stricmp(filename, "models/player/spetsnaz/spetsnaz.mdl") ? false : true;

	if (crc == 0xEC43F76D || crc == 0x270FB2D7)
		return Q_stricmp(filename, "models/player/terror/terror.mdl") ? false : true;

	if (crc == 0x1AAA3360 || crc == 0x35AC6FED)
		return Q_stricmp(filename, "models/player/gign/gign.mdl") ? false : true;

	if (crc == 0x02B95E5F || crc == 0x72DB74E4)
		return Q_stricmp(filename, "models/player/vip/vip.mdl") ? false : true;

	if (crc == 0x1F3CD80B || crc == 0x1B6C4115)
		return Q_stricmp(filename, "models/player/guerilla/guerilla.mdl") ? false : true;

	if (crc == 0x3BCAA016)
		return Q_stricmp(filename, "models/player/militia/militia.mdl") ? false : true;

	if (crc == 0x43E67FF3 || crc == 0xF141AE3F)
		return Q_stricmp(filename, "models/player/sas/sas.mdl") ? false : true;

	if (crc == 0xDA8922A || crc == 0x56DD2D02)
		return Q_stricmp(filename, "models/player/gsg9/gsg9.mdl") ? false : true;

	if (crc == 0xA37D8680 || crc == 0x4986827B)
		return Q_stricmp(filename, "models/player/arctic/arctic.mdl") ? false : true;

	if (crc == 0xC37369F6 || crc == 0x29FE156C)
		return Q_stricmp(filename, "models/player/leet/leet.mdl") ? false : true;

	if (crc == 0xC7F0DBF3 || crc == 0x068168DB)
		return Q_stricmp(filename, "models/player/urban/urban.mdl") ? false : true;

	return false;
}
