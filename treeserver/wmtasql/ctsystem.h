#ifndef __BTSYSTEM_H
#define __BTSYSTEM_H

/*
 *   entry points for the memory functions
 */


//extern int rename();          /* change the name of a file                  */
//extern char *memcpy();        /* move a block of memory to a new spot       */
//extern char *memset();        /* set a block of memory to a given value     */
//extern int  unlink();         /* delete a file by a given name              */


short	exist(char *fName);

void 	ltoc(register long j, char *buff);
long 	ctol(char *buff);
void 	ltoc(long j, char *buff);
short 	rmsign(short c);

#endif

/*
 *  end of system.h
 */



