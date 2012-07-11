//
// by Jingyu Niu 1999.09
//
#ifndef __INC_PLUG_INS__
#define __INC_PLUG_INS__

#pragma pack(push,1)

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PLUGINS_NAME		255
#define MAX_PLUGINS_PARAMETER	1024
#define MAX_PLUGINS_PROVIDER	512
#define MAX_PLUGINS_ENTRY_NAME	128

struct tagPlugIns;

typedef struct tagPlugIns PLUGINS_CONTEXT, *PPLUGINS_CONTEXT, *LPPLUGINS_CONTEXT;
typedef DWORD (WINAPI *PLUGINS_PROC)( LPPLUGINS_CONTEXT );

typedef enum {
	tspNotLoad,
	tspLoadFail,
	tspIncorrectImage,
	tspLoadSucceed
} TS_PLUGINS_STATUS, PTS_PLUGINS_STATUS, LPTS_PLUGINS_STATUS;

struct tagPlugIns {
	TCHAR	szName[MAX_PLUGINS_NAME];
	TCHAR	szImage[MAX_PATH];
	TCHAR	szParameter[MAX_PLUGINS_PARAMETER];
	TCHAR	szProvider[MAX_PLUGINS_PROVIDER];
	HINSTANCE hLibrary;
	PLUGINS_PROC lpPlugInsMain;
	TS_PLUGINS_STATUS tspStatus;
	LPVOID	lpSysUse1;		// Used by System. User Can Not
	LPVOID	lpSysUse2;		// Change it.
};

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif //__INC_PLUG_INS__