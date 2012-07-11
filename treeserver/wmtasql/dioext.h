/****************
*				dioext.h
*    Copyright (c) 1994,1995
*****************************************************************************/

#ifndef __DIOEXT_
#define __DIOEXT_

#ifndef _IncludedDIOProgramHead_
	#include "dio.h"
#endif
#ifndef BtreeHeadFileIncluded
#include "btree.h"
#endif

extern char *dioTmpFile;
extern short   confirm;

typedef struct tagdFILEENV {
	long int rec_p;
} dFILEENV;

long viewToDbf( char *view, char *dbf );
char *getViewDbfname( char *view );
dFILE *getdViewViewdbf( dFILE *df );

short dFILEcmp( dFILE *df1, dFILE *df2 );
short dwriteDbfInfo( dFILE *df, char *path );
dFILE *dreadDbfInfo( dFILE *df, char *path );

long dbfCopy( dFILE *sdf, dFILE *tdf );

long dbfFlagCopy( char *sdbf, char *tpath, char *flagFld );

short dAccess( char *filename, short amode );

short tgCarryDbfRec( dFILE *tdf, dFILE *sdf );

dFILE *dModiCreate( char *filename, dFIELD field[] );

int dIsAwake( char *dbfName );

dFILE *joinCreateDbf( short DbfNum, char *TargetDbf, char *dbf[] );

long dAppendFile(char *sDbf, char *tDbf, bHEAD *bh, short uniqueOrFree);

#ifdef DOS_COMPATABLE
unsigned short batchAppend( char *spath, char *files, char *dpath );
#endif

long dsetFldFlag( char *sdbf, char *flagFld, char *cont );

dFILEENV *saveDfileEnv(dFILE *df, dFILEENV *env);
dFILE *setDfileEnv(dFILE *df, dFILEENV *env);
long recDelPack(dFILE *df, bHEAD *bh);

dFILE *dTmpCreate(dFIELD field[]);
short dTmpClose(dFILE *df);
int checkRecValid(dFILE *df);

#endif
