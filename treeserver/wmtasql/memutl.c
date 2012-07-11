/****************
*			memutl.c
*  Copyright (c) 1994
****************************************************************************/

#include <stdarg.h>
#ifdef _MSC_VER
#include <memory.h>
#else
#include <mem.h>
#endif
#include <LIMITS.H>
#include <stdlib.h>

char *memNewBlockOnce(char **ss, unsigned short memSize, ... )
{
    va_list ap;
    // unsigned short memSize1;		// Remed by NiuJingyu
    char **ss1;
    char *pool;
    unsigned short poolSize, count;

    poolSize = memSize;

    // count mem alloc
    va_start(ap, memSize);
    while( (ss1 = va_arg(ap, char **)) != NULL ) {
	poolSize += va_arg(ap, unsigned short);
    }
    va_end(ap);
    if( poolSize <= 0 )	return  0;

    if( (pool = malloc( poolSize ) ) == NULL ) 	return  0;
    memset( pool, 0, poolSize );

    *ss = pool;
    count = memSize;

    va_start(ap, memSize);
    while( (ss1 = va_arg(ap, char **)) != NULL ) {
	*ss1 = (char *)(pool + count);
	count += va_arg(ap, unsigned short);
    }
    va_end(ap);

    return pool;

} // end of function memNewBlockOnce()


/****************
*			tryMalloc()
*****************************************************************************/
char *tryMalloc( unsigned int *size, unsigned int minSize )
{
    char *p;

    do {

	p = (char *)malloc( *size );
	if( p == NULL ) {
		*size /= 2;
	} else {
		return  p;
	}

    } while( p == NULL && *size >= minSize );

    return  NULL;

} // end of tryMalloc()


/****************
*			zeroMalloc()
*****************************************************************************/
void *zeroMalloc(size_t size)
{
    void *p;
    p = malloc(size);
    if( p != NULL ) {
	memset(p, 0, size);
    }
    return  p;
}


/*

main()
{
	char *s1, *s2, *s3, *s4, *s;

	s = memNewBlockOnce(&s1, 10, &s2, 20, &s3, 30, &s4, 40, NULL );

	printf("%s\n", s);
}

*/