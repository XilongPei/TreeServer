
#pragma pack(push, 4)

#ifndef LOGIN_INCL
#define LOGIN_INCL

#define	NAME_LEN  19
#define PASS_LEN  19
#define ENVI_LEN  129
#define DATE_LEN  6	
#define TEMP_LEN  81
#define ANSI_LEN  1
#define NL_LEN     3
#define LC_LEN	  9
/*This type is for saving LOGIN variables for communication between FE and BE*/
typedef struct
	{
	char Host[NAME_LEN];
	char cHost;
	char User[NAME_LEN];
	char cUser;
	char Pass[PASS_LEN];
	char cPass;
	char Service[NAME_LEN];
	char Protocol[NAME_LEN];
	char InetType;	
	char DbPath[ENVI_LEN];
	char DbDate[DATE_LEN];
	char DbMoney[NAME_LEN];
	char DbTime[TEMP_LEN];
	char DbTemp[TEMP_LEN];
	char DbLang[NAME_LEN];
	char DbAnsiWarn[ANSI_LEN];
	char InformixDir[TEMP_LEN];
	char DbNls[NL_LEN];
	char CollChar[NL_LEN];
	char Lc_Ctype[LC_LEN];
	char Lc_Collate[LC_LEN];
	} LoginInfoStruct;

extern LoginInfoStruct InetLogin;

#endif /* LOGIN_INCL */

#pragma pack(pop, 4)

