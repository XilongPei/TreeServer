#ifndef __PUBVAR_H_
#define __PUBVAR_H_

#include "dir.h"

#ifdef __N_C_S
#include "dialog.h"
#include "tg_flag.h"
extern StaticText  bRuningText;
#endif

// porgram run parameter
extern char **argvSaved;
extern char **envSaved;
extern int  argcSaved;


extern char *ctlres;		// define in read.c

extern char *eucfg;
extern char szHelpFile[MAXPATH];
extern char *frompath;
extern char *datapath;
extern char *fieldctl;
extern char *cfname;
extern char *szTbcFile;
extern char *szTbFile;
extern char *tbpath;
extern char *tbcpath;
extern char *tgask;
extern char *e_expr;
extern char *c_expr;
extern char *c_brif;
extern char *codepath;
extern char *STRRES;
extern char *szCxmain;
extern char szSub0[9];
extern char *szFilectl;
extern char *szFlag;
extern char *s;
extern char *szPubDatapath;

extern char *tmpFile;
extern char *bakFile;

extern char pubBuf[1024];

extern char pubBuf0[81];
extern char pubBuf1[81];
extern char pubBuf2[81];
extern char pubBuf3[81];
extern char pubBuf4[81];
extern char pubBuf5[81];
extern char pubBuf6[81];
extern char pubBuf7[81];
extern short pubShort0;
extern short pubShort1;

short initPubVar( void );


#endif
