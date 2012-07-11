/*-------------------------------------------*
 *
 *   FileName: diodbt.h
 *   Function: read memo filed from DBF file to another
 *                      text file for edit and put it back;
 *
 *   Author:   Zhaolin Shan
 *   Date:           5/4/1995
 *-------------------------------------------------------------------------*/

#ifndef __DIODBT_H__
#define __DIODBT_H__


long  dbtToFile( dFILE *df, unsigned short fldId, char *filename );
long  dbtFromFile( dFILE *df, unsigned short fldId, char *fileName );

void *blobToMem(dFILE *df, unsigned short fldId, long *memSize);
long  blobFromMem( dFILE *df, unsigned short fldId, char *mem, long memSize );

void freeBlobMem( dFILE *df, char *mem );

#endif