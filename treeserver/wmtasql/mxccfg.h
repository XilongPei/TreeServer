/*-=-=-=-=-=-=-=-=
 *
 *   MXCCFG.H
 *
 *      Borland C/3.1 Version
 *
 *   Copyright (c) MX Groups 1993
 *   copyright (c) EastUnion Computer Service Co., Ltd. 1995
 *
 * Caution:
 *   the max size of config file is 64K
 *
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifndef __MXCCFG_H
#define __MXCCFG_H

#include <stdio.h>

typedef long  hPOS;

#define c_markChar "[]="

// cause error when runing;
#define e_failure  -1L

// line max length
#define  cacheSize  256

extern hPOS cfgHeadSize;

typedef struct
{
     FILE *hCFG;
     int   cfgHeadSize;
     hPOS  seg;
     hPOS  segEnd;
} MXCCFG;

/*
 * ----------------------------------------------------------------------
 *               Local Function prototype
 * ----------------------------------------------------------------------
 */
MXCCFG *cfg_create( char *file, int cfgHeadSize );
MXCCFG *cfg_open( char *cfg, int cfgHeadSize );
void  cfg_close( MXCCFG *ch );

// locate the config segment, fail e_failure
hPOS cfg_segment( MXCCFG *ch, char *grpKey, char *label );

//return: fail, 0; success, not 0
char *cfg_read( MXCCFG *ch, char *cfgKey, char *buf );

// overwrite
// 1: overate 2:append 0:insert
int   cfg_write( MXCCFG *ch, char *cfgKey, char *text, int  overwrite);

// not use this function if you don't know it how to run.
hPOS  cfg_getPos( MXCCFG *ch );

// success return 0
int   cfg_setSeg( MXCCFG *ch, hPOS newseg );


// generate a new segment
int   cfg_make( MXCCFG *ch, char *grpKey, int  rj );


void  cfg_writeLine( MXCCFG *ch, char *memo );
int   cfg_readLine( MXCCFG *ch, char *linebuf, int maxsize );

#endif
