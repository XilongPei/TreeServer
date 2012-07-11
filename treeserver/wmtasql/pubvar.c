#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "dir.h"

#include "mistring.h"
#include "pubvar.h"
#include "strutl.h"
#include "cfg_fast.h"

char *eucfg = "EUCFG";
char szHelpFile[MAXPATH];
char *frompath = "FROMPATH";
char *datapath = "DATAPATH";
char *codepath = "CODEPATH";
char *fieldctl = "FIELDCTL";
char *cfname = "CFNAME";
char *szTbcFile = "TBC_FILE";
char *szTbFile = "TB_FILE";
char *tbpath = "TBPATH";
char *tbcpath = "TBCPATH";
char *e_expr = "E_EXPR";
char *c_expr = "C_EXPR";
char *c_brif = "C_BRIF";
char *tgask = "tgask";
char *STRRES = "strres";
char *szCxmain = "CXMAIN";
char szSub0[9];
char *szFilectl = "FILECTL";
char *szFlag    = "FLAG";
char *s;

char *tmpFile = "ILOVETMP.TMP";
char *bakFile = "ILOVEBAK.TMP";

#ifdef __N_C_S
StaticText              bRuningText;
#endif

char pubBuf[1024];

char pubBuf0[81];
char pubBuf1[81];
char pubBuf2[81];
char pubBuf3[81];
char pubBuf4[81];
char pubBuf5[81];
char pubBuf6[81];
char pubBuf7[81];
short pubShort0;
short pubShort1;

char **argvSaved;
char **envSaved;
int  argcSaved;


short initPubVar( void )
{
    s = GetCfgKey(csu, "BASESET", NULL, "BASESETS");
    assert( s );
    ltrim(s);
    s = strZcpy(szSub0, s, 9);
    while( isalnum( *s ) )              s++;
    *s = '\0';

    s = GetCfgKey(csu, eucfg, NULL, "HELP");
    if( s != NULL ) {
	lrtrim( s );
	strZcpy(szHelpFile, s, MAXPATH);
    }

    return  1;

}


/********************** end of module pubvar.h ***************************/
