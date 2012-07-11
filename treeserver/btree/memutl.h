/****************
*			memutl.h
*  Copyright (c) 1995
****************************************************************************/

#ifndef __MEMUTL_H__
#define __MEMUTL_H__

char *memNewBlockOnce(char **ss, unsigned short memSize, ... );
char *tryMalloc( unsigned int *size, unsigned int minSize );
void *zeroMalloc(size_t size);

#endif
