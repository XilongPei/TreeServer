//
// by Jingyu Niu 1999.09
//
#ifndef __INC_TS_PLUG_INS__
#define __INC_TS_PLUG_INS__

#include "plug-ins.h"

#pragma pack(push,1)

#ifdef __cplusplus
extern "C" {
#endif

// Error no used by plug-ins
#define TERR_NO_PLUGINS			8001

typedef struct tagPlugIns TS_PLUGINS, *PTS_PLUGINS, *LPTS_PLUGINS;

typedef LPTS_PLUGINS LPTS_PLUGINS_ENTRY;

extern LPTS_PLUGINS_ENTRY lpPlugInsEntry;

DWORD InitPlugIns ( VOID );
VOID UnloadPlugIns ( VOID );

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif //__INC_TS_PLUG_INS__