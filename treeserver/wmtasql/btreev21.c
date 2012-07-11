/*
 *   SoftFocus BTree Library: v2.0
 *
 *      Copyright 1984, 1985 by Softfocus
 *                              1343 Stanbury Dr.,
 *                              Oakville, ONT.
 *                              Canada, L6L 2J5
 *                              (416) 825 - 0903
 *
 *   Written by L.Karnis from Dec., 83 to July 84
 *
 *   Remember your license states that you can not redistribute the
 *   source, manual or make public the object code with the entry
 *   points. You can use this software in any application, flog that
 *   and you don't owe us a penny. Please keep us informed if you
 *   find any bugs or make any significant enhancements.
 *
 *  Version 2.0     Released 85.05.01
 *
 *  The significant difference between this and earlier
 *  versions is the support of duplicate keys. Some algorithms have been
 *  streamlined and some (deeply buried) bugs exterminated.
 *
 *  We have tested this package with 60,000 records, key sizes up to
 *  80 bytes long and data records up to 10K bytes long.
 *
 *  jon simkins, 85.04.22
 *
 *  Date        Revision History
 *  ----        -------- -------
 *
 *  85.04.28    Fixed up functions that copied new key values to
 *              NOT return the duplicate bytes.
 *              mem2cpy no longer used and kkSize() added. j.s.
 *
 *  85.05.04    Fixed lioAvl() and lioFree() to release malloc()ed
 *              space in those routines in case of btRead() or
 *              btWrite() failure. js.
 *
 *  85.05.22    (How did we miss this one??)
 *              Fixed bug in lioSearch() that would cause search
 *              failure of duplicate keys after the first one entered
 *              had been deleted. Changed ndxCmp1() to take an explicit
 *              byte count instead of reading vindex[]. js
 *
 *  85.05.23    Optimization to btSearch() & btDelete() to factor out
 *              unnecessary ndxCmp() calls. js (suggested by David
 *              Caulfield)
 *
 *  85.06.07    Changed ctol() and ltoc() to use shift instructions
 *              instead of long multiplies. Old routines commented
 *              out (just in case). js.
 *
 *  85.06.17    Added correct declaration for memset() and memcpy();
 *              this solved problems under C86 big model. Also added
 *              cast to second argument in memset(). js.
 *
 *  85.07.30    Fixed bug in lioOpen() that could close stdout if the
 *              first file open operation failed. thank you Steve. js.
 *
 *  85.09.19    Changed lioSerNum() to conform with the manual description.
 *              js.
 *
 *  85.09.30    Fixed lioSerNum() which would occaisionally cause
 *              spurious 'no key for data record' in ISAM.
 *              Linted and cleaned up formatting using Wizard's C
 *              comprehensive diagnostics! js.
 *
 *  85.10.07    Fixed lioSerNum() to work correctly (added kkSize()).
 *              js. I hate lioSerNum().
 *
 *  85.10.11    Fixed a bug in ndxCmp(), where it was giving the first
 *              duplicate in a tree the same duplicate number as the
 *              original. larry karnis (yes I'm still alive!)
 *
 *  85.10.14    Added conditional compilation of rmsign() routines in
 *              ctol() for compilers that sign extend chars. js.
 *
 *  85.10.18    Added (short) casts in inxCmp1() for DeSmet. Added char
 *              sign extension logic into inxCmp1() (probably paranoid
 *              code!) js.
 *
 *  85.12.20    lioOpen now copies the name the file was opened with into
 *              the header block. Could cause problems if the file was
 *              renamed and lioFlush()ed. Thanks Dave! Merry Christmas.js.
 *
 *  86.01.22    Cut ltoc() and ctol() out of BTree and put them in system.c.
 *              Fixed (tiny) problem with lioDelete() that could fail in
 *              certain cases when called with an explicit key. js.
 *
 *  86.03.04    lioGetCur() and lioPutCur() improved and tested. Still
 *              formally undocumented, but now working. js.
 *              Program carefully reformatted. js.
 *              uTag union pulled out; only struct detail now used for
 *              header read/writes. js.
 *  86.03.20    Added a lru cache algorithm for tree pages. Speen increase
 *              of 10-30% depending on operation. js.
 *              Fixed bug in search partial that could find next largest
 *              key even when given an exact match. thanks, bob. js.
 *  
 */
#include <stdio.h>  /* compiler manual, also common defines                 */
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <process.h>

#include "ctsystem.h"   /* entry points for system dependent functions      */
#include "ctree.h"


LOCAL short initialized = (short) NULL; /* used to initialize the internal tables */

//extern short strcmp();
//extern long lseek(), ctol();
//extern void free();
//extern char *malloc(), *memcpy(), *memset();

/*
 *  detail for open files lives here
 */
LOCAL struct btTail detail;
LOCAL struct indexTag vindex[NUMINDEXS];

LOCAL short hasOver;               /* has the tree overflowed                */
LOCAL short amInserting;           /* true while actually inserting a key    */
LOCAL struct PAGE rootPg;          /* workspace for a new root page          */
LOCAL struct ITEM currItem;        /* temporary item for inserting a key     */
LOCAL char currBufr[MAXKEYSIZE+1]; /* more temporary key space               */
LOCAL char keyValue[MAXKEYSIZE+1]; /* current key being operated on          */
LOCAL short adjFlag;     /* true if something is being passed back up        */
LOCAL long btResult;     /* status word for success or failure of an op'n    */
LOCAL long keyRcdNo;     /* data file record number for what is being opd on */
LOCAL struct {
    short pgInUse;
    char *pgPtr;
} allocPage[MAXTREEHEIGHT*2];

/*
 *  cache details
 *
 *  Our tests here (on an 8mhz 266 MS-DOS machine) indicate that 5 is
 *  a near-optimum number of slots; you may wish to experiment with your
 *  own flavour of hardware.
 *
 *  Setting MAXCACHE to 1 effectively disables all cache operations.
 */
LOCAL unsigned lru = 1;
LOCAL struct btCache cache[MAXCACHE];

LOCAL short kkSize(short btDes);
short lioCacheResize(short keyDes, short slot);


/* ------------------------------------------------------------------------ */

/*
 *  initialize the in memory tables. Remains an entry point for compatibility.
 */

ENTRY void lioInit( void )
{
   if (initialized)
       return;
   (void)memset((char *)vindex, (char) NULL, sizeof(vindex));
   (void)memset((char *)allocPage, (char) NULL, sizeof(allocPage));

   /*add by marlin on 9,21,1994*/
// (void) memset( (char *) cache,     (char) NULL, sizeof(cache));

   initialized = TRUE;
   currItem.srchVal = currBufr;         /* set up some temp. key space */
}

/*
 *  The following two routines are used to save and restore the BTree
 *  currency. The structure 'currency' is defined in btgconst.h.
 *
 *  copy out the current currency values
 */
ENTRY void
lioGetCur(keyDes, p)
short keyDes;
struct currency *p;
{
    if (!initialized)
        lioInit();
    p -> sPtr = vindex[keyDes].stackPtr;
    (void) memcpy((char *) p -> stk, (char *) vindex[keyDes].stack,
                    sizeof(long) * MAXTREEHEIGHT );
    (void) memcpy(p -> last, vindex[keyDes].matchName, vindex[keyDes].keySize);
}

/*
 *   replace the currency information
 */
ENTRY void
lioPutCur(keyDes, p)
short keyDes;
struct currency *p;
{
    if (!initialized)
        lioInit();
    vindex[keyDes].stackPtr = p -> sPtr;
    (void) memcpy((char *) vindex[keyDes].stack, (char *) p -> stk, 
                    sizeof(long) * MAXTREEHEIGHT );
    (void) memcpy(vindex[keyDes].matchName, p -> last, vindex[keyDes].keySize);
}

/*
 *    return the number of keys/records for a data or index file
 */
ENTRY long
lioSize(keyDes)
register short keyDes;

{
    if (!initialized)
        lioInit();
    return (vindex[keyDes].keyType == tDATA) ? vindex[keyDes].numRecs :
                         vindex[keyDes].nKeys;
}

/*
 *   return the size of a 'unit' of storage.
 */
ENTRY short lioPSize(register short keyDes)
{
    if (!initialized)
        lioInit();
    return vindex[keyDes].keyPgSz;
}

/*
 *   return a boolean that says whether duplicates are allowed
 */
ENTRY short
lioDup(keyDes)
short keyDes;
{
    if (!initialized)
        lioInit();
   return vindex[keyDes].hasDup;
}

/*
 *   get the logical record number of the next free record
 */
ENTRY long
lioAvl(keyDes)
register short keyDes;
{
    register long result;
    char *ptr;

    if (!initialized)
        lioInit();
    vindex[keyDes].numRecs++;

    /*
     *  see if there's anythin on the free list
     */
    if (vindex[keyDes].nextAvl != (short) NULL) {
        result = vindex[keyDes].nextAvl;
        if ((ptr = malloc(vindex[keyDes].keyPgSz)) == (char *) NULL)
            return LERROR;
	if (lioRead(keyDes, result, ptr) == (short) NULL) {
            free(ptr);
            return LERROR;
        }
        (void) memcpy((char *) &vindex[keyDes].nextAvl, ptr, sizeof(long));
        free(ptr);
        return result;
    }
    return vindex[keyDes].numRecs;
}

/*
 *  close an index file
 */
ENTRY short
lioClose(short keyDes)
{
    if (!initialized)
        lioInit();
    if (lioFlush(keyDes))
        return ERROR;
    if (close(vindex[keyDes].fd)) 
        return ERROR;

    /*
     *  NULL out the files' old entry in the control table
     */
    vindex[keyDes].keySize = (short) NULL;
    vindex[keyDes].keyType = (short) NULL;
    vindex[keyDes].keyOrder = (short) NULL;
    vindex[keyDes].keyPgSz = (short) NULL;
    vindex[keyDes].numRecs = LNULL;
    vindex[keyDes].nKeys = LNULL;
    vindex[keyDes].fd = (short) NULL;
    return (short) NULL;
}

/*
 *  flush an index or data file
 */
ENTRY short lioFlush(short keyDes)
{
    /*
     *  flush the cache
     */
    if (vindex[keyDes].keyType != tDATA)
	if (flCache() == ERROR)
            return ERROR;

    /*
     *   when a file is released, reset the inUse flag to indicate that
     *  file operations were terminated normally.
     */
    if (!initialized)
        lioInit();

    if (lseek(vindex[keyDes].fd, LNULL, SEEK_SET) == LERROR)
        return ERROR;
    if (read(vindex[keyDes].fd,(char *) &detail, sizeof(detail))
                                                          != sizeof(detail))
        return ERROR;


    /*
     *  update the file header block values
     */
    (void) memset(detail.lioName, (char) NULL, NAMESIZE);
    (void) memcpy(detail.lioName, vindex[keyDes].fName, NAMESIZE);
    detail.freeNum = vindex[keyDes].nextAvl;  
    detail.numRcds = vindex[keyDes].numRecs;     
    detail.numKeys = vindex[keyDes].nKeys;   
    detail.rootNum = vindex[keyDes].treeRoot;    
    detail.dupVal = vindex[keyDes].dupNum;   
    detail.inUse = vindex[keyDes].trInUse = FALSE; 

    if (lseek(vindex[keyDes].fd, LNULL, SEEK_SET) == LERROR)
        return ERROR;
    if (write(vindex[keyDes].fd, (char *) &detail, sizeof(detail))
                                                     != sizeof(detail))
        return ERROR;

    /*
     *  only safe GENERIC way to ensure file is on disk using MS-DOS; why
     *  isn't sync() in the operating system somewhere?
     */
    close(vindex[keyDes].fd);
    vindex[keyDes].fd = open(detail.lioName, RWMODE);

    return (short) NULL;
}

/*
 *    Initialize a new index file.
 */
ENTRY short
lioCreat(fName, keySiz, keyTyp, dupAllowed)
short keySiz, keyTyp;
char *fName, dupAllowed;
{
    int  fd;

    if (!initialized)
        lioInit();
    if (keySiz > MAXKEYSIZE-3 && keyTyp == tSTRING && dupAllowed)
        return ERROR;
    (void) memset((char *) &detail, (char) NULL, sizeof(detail));
    (void) memcpy(detail.lioName, fName, NAMESIZE);
    if (exist(detail.lioName)) 
	return ERROR;

    /*	modified by Marlin on 9,21,1994	*/

    /*
    if ((fd = creat(detail.lioName, CREATMODE))==NOCREAT)
	return ERROR;
    */

    if ((fd = open(detail.lioName, CREATMODE))==NOCREAT)
	return ERROR;

    detail.order = MINKEYS;      /* b-tree order (for safe keeping) */
    if (dupAllowed && keyTyp == tSTRING) keySiz += 3;
    detail.kSize = keySiz;
    detail.kType = keyTyp;   /* either tDATA or tSTRING         */
    detail.dupOk = dupAllowed;   /* remember if duplicates are ok   */

    /*
     * calculate the number of bytes required for each page 
     * of the key tree
     */
    detail.pageSize = (keyTyp == tDATA) ? keySiz :
        (keySiz+2*(sizeof(long)-1))*MAXKEYS+(sizeof(long)-1)+sizeof(short);

    /*
     *add by marlin on 9,21,1994
     */
    detail.pageSize = (detail.pageSize + BLOCKSIZE)/BLOCKSIZE*BLOCKSIZE;

    /*
     *  write the file header
     */
    if (write(fd, (char *) &detail, sizeof(detail)) != sizeof(detail))
        return ERROR;
    if (close(fd)) 
        return ERROR;

   return (short) NULL;
}

/*
 *  return an allocated logical record number to the free list
 */
ENTRY short
lioFree(keyDes, discard)
short keyDes;
long discard;
{
    char *ptr;

    if (!initialized)
        lioInit();
    if ((ptr = malloc(vindex[keyDes].keyPgSz)) == (char *) NULL) 
        return ERROR;
    (void) memset(ptr, (char) NULL, vindex[keyDes].keyPgSz);
    (void) memcpy(ptr, (char *) &vindex[keyDes].nextAvl, sizeof(long));
    if (lioWrite(keyDes, discard, ptr) == (short) NULL) {
        free(ptr);
        return ERROR;
    }
    vindex[keyDes].numRecs--;
    vindex[keyDes].nextAvl = discard;
    free(ptr);

    return (short) NULL;
}


/*
 *  lioOpen - open an existing key or data file.
 */
ENTRY short lioOpen(char *fName)
{
    register short i, keyDes;

    if (!initialized) 
        lioInit();

    /*
     *  find the first empty file control block
     */
    (void) memset( (char *) &detail, (char) NULL, sizeof(detail));
    for(i = 0; i < NUMINDEXS; i++)
	if (vindex[i].fd == (short) NULL)
	    break;
    if ((keyDes = i) == NUMINDEXS)
	return ERROR;

    /*
     *  open the file and read the header
     */
    if ((vindex[keyDes].fd = open(fName, RWMODE)) == NOPEN) {
	vindex[keyDes].fd = (short) NULL;
	return ERROR;
    }
    lseek(vindex[keyDes].fd, 0, SEEK_SET);
    if( read(vindex[keyDes].fd, (char *)&detail, sizeof(detail))
						    != sizeof(detail))
	return ERROR;

    /*
     *  check that the file is uncorrupted
     */
    if (detail.inUse) {
	(void) close(vindex[keyDes].fd);
	vindex[keyDes].fd = (short) NULL;
	return ERROR;
    }

    /*
     *  set up the in-memory table
     */
    (void) memcpy(vindex[keyDes].fName, fName, NAMESIZE);
    vindex[keyDes].nextAvl = detail.freeNum;
    vindex[keyDes].numRecs = detail.numRcds;
    vindex[keyDes].nKeys = detail.numKeys;
    vindex[keyDes].keySize = detail.kSize;     
    vindex[keyDes].keyType = detail.kType;
    vindex[keyDes].keyPgSz = detail.pageSize;
    vindex[keyDes].treeRoot = detail.rootNum;
    vindex[keyDes].hasDup = detail.dupOk;
    vindex[keyDes].dupNum = detail.dupVal;

    return keyDes;
}

/*
 *  get a logical page from the file fd
 */
ENTRY short lioRead(short keyDes, long pageNo, char *dest)
{
    long offset;
    int   i;
    unsigned int   ii;

    if (!initialized)
	lioInit();

    offset = (pageNo - 1) * vindex[keyDes].keyPgSz + (long)sizeof(detail);
    /*if (lseek(vindex[keyDes].fd, offset, SEEK_SET) == LERROR)
	return (short) NULL;*/
    i = lseek(vindex[keyDes].fd, offset, SEEK_SET);
    ii = vindex[keyDes].keyPgSz;
    i = read(vindex[keyDes].fd, dest, ii);
/*in Watcom C, int deal with short will be error
    if( i != vindex[keyDes].keyPgSz)
	return (short) NULL;
*/
    return vindex[keyDes].keyPgSz;
}

/*
 *  put a logical page to the file fd
 */
ENTRY short
lioWrite(keyDes, pageNo, src)
register short keyDes;
register long pageNo;
char *src;
{
    long offset;

    if (!initialized)
        lioInit();

    if (vindex[keyDes].trInUse == FALSE) {
        vindex[keyDes].trInUse = TRUE;
        if (markFile(keyDes)) 
	    return (short) NULL;
    }
    offset = sizeof(detail) + (pageNo - 1) * vindex[keyDes].keyPgSz;
    if (lseek(vindex[keyDes].fd, offset, SEEK_SET) == LERROR)
	return (short) NULL;
    if (write(vindex[keyDes].fd, src, vindex[keyDes].keyPgSz) !=
                                                    vindex[keyDes].keyPgSz)
	return (short) NULL;
    return vindex[keyDes].keyPgSz;
}

/*
 *  kkSize  Return the size of the actual index keysize MINUS any
 *          duplicate key bytes.
 */
LOCAL short
kkSize(btDes)
short btDes;
{
    return (vindex[btDes].hasDup) ? vindex[btDes].keySize - 3 :
                                                vindex[btDes].keySize;
}

ENTRY short
lioKsize(btDes)
short btDes;
{
    return kkSize(btDes);
}

ENTRY short lioCacheResize(short keyDes, short slot)
{
    /*
     *	add by marlin on 9,21,1994
     *
    */
    if (vindex[keyDes].keyPgSz > cache[slot].cacheSize)
    {
	char 	*temp;

	temp = malloc(vindex[keyDes].keyPgSz);
	if (temp == NULL)
	    return ERROR;

	memcpy(temp,cache[slot].temp,cache[slot].cacheSize);
	free(cache[slot].temp);
	cache[slot].cacheSize = vindex[keyDes].keyPgSz;
	cache[slot].temp = temp;
    }
    return  0;
}

/*
 *  rdCache     Return pointer to the requested page. Update
 *              the cache if necessary.
 */
LOCAL char *rdCache(short keyDes, long pageNo)
{
    register j;
    unsigned last, slot;

    /*
     *  see if the page in is the cache already
     */
    for(j=0, last=lru, slot=0; j<MAXCACHE; j++) {
        if ((cache[j].cRecNum == pageNo) && (cache[j].cfd == keyDes) ) {
	    cache[j].lru = lru++;
            return cache[j].temp;
	}
        if (last > cache[j].lru) {
	    last = cache[j].lru;
            slot = j;
        }
        if (cache[slot].cRecNum == 0L)
	    break;
    }

    //add by marlin
    if (lioCacheResize(keyDes, (short)slot) == ERROR)
	return (char *) NULL;

    /*
     *  write dirty slot
     */
    if (cache[slot].dirty)
	if (lioWrite(cache[slot].cfd, cache[slot].cRecNum, cache[slot].temp)==(short) NULL)
	    return (char *) NULL;

    /*
     *  read the given record and update the cache
     */

    if (lioRead(keyDes, pageNo, cache[slot].temp) == (short) NULL)
	return (char *) NULL;

    cache[slot].cRecNum = pageNo;
    cache[slot].cfd = keyDes;
    cache[slot].dirty = FALSE;
    cache[slot].lru = lru++;

    return cache[slot].temp;
}

/*
 *  wrtCache        Return a pointer to where the given page should be written.
 *                  Flush the cache as necessary.
 */
LOCAL char *
wrtCache(keyDes, pageNo)
short keyDes;
long pageNo;
{
    register j;
    unsigned slot, last;                                                        

    /*
     *  see if the page in is the cache already
     */
    for(j=0, last=lru, slot=0; j<MAXCACHE; j++) {
        if ( (cache[j].cRecNum == pageNo) && (cache[j].cfd == keyDes) ) {
            slot = j;
            cache[slot].dirty = FALSE;
            break;
        }
        if ( (last > cache[j].lru) ) {
            slot = j;
            last = cache[j].lru;
        }
        if (cache[slot].cRecNum == 0L)
            break;
    }

    //add by marlin
    if (lioCacheResize(keyDes,(short)slot) == ERROR)
	return (char *) NULL;

    /*
     *  write slot (if necessary) and update cache entries
     */
    if (cache[slot].dirty)
	if (lioWrite(cache[slot].cfd, cache[slot].cRecNum, cache[slot].temp)==(short) NULL)
            return (char *) NULL;

    cache[slot].lru = lru++;
    cache[slot].cRecNum = pageNo;
    cache[slot].cfd = keyDes;
    cache[slot].dirty = TRUE;

    return cache[slot].temp;
}

/*
 *  flCache     Flush all dirty buffers to disk. Reset lru to the
 *              beginning. Clear the cache.
 */
LOCAL short flCache(void)
{
    register j;

    for(j=0; j<MAXCACHE; j++) {
	if (cache[j].cRecNum == 0L)
	    break;
	if (cache[j].dirty)
	    if (lioWrite(cache[j].cfd, cache[j].cRecNum, cache[j].temp) == (short)NULL)
		return ERROR;
    }
    memset((char *)cache, (char) 0, sizeof(cache));
    lru = 1;

    return OK;
}   

/*
 *  lioReadKey  read a page from the disk, get space for all active
 *              keys and install pointers to them in the node.
 *short keyDes;                      index into key descriptor table
 *long pageNo;                     logical page to read
 *struct PAGE *dest;               where to put the adjusted node
 */
LOCAL short lioRdKey(short keyDes, long pageNo, struct PAGE *dest)
{
    char *dSpace, *nodAlloc() ;
    char *buff;
    register short i, len;

    if ((buff = rdCache(keyDes, pageNo)) == (char) NULL) 
        return ERROR;

    len = vindex[keyDes].keySize;

    dSpace = (char *)nodAlloc((short)(MAXKEYS * (len+1)));
    dest -> nodSiz = *buff++;
    dest -> lftSon = ctol(buff);
    buff += 3;

    for(i = (short) NULL; i < MAXKEYS; i++) {
        dest -> nodElmt[i].srchVal = dSpace + i * (len+1);
        if (i < dest -> nodSiz) {
            memcpy(dest -> nodElmt[i].srchVal, buff, len);
            buff += len;
            dest -> nodElmt[i].child = ctol(buff);  /* get sub-tree pointer */
            buff += 3;
            dest -> nodElmt[i].spare = ctol(buff);  /* & the data record #  */
            buff += 3;
        }
    }
    return (short) NULL;
}

/*
 *  lioWriteKey     write a page to the disk, rebuild the node before 
 *                  writing.
 */
LOCAL short
lioWrtKey(keyDes, pageNo, src)
short keyDes;                     /* index into key descriptor table  */
long pageNo;                    /* logical page to read             */
struct PAGE *src;               /* where to put the adjusted node   */
{
    char *buff ;
    short i, len;

    if ( (buff = wrtCache(keyDes, pageNo)) == (char) NULL)
        return ERROR;

    len = vindex[keyDes].keySize;
    *buff++ = (char)(src -> nodSiz);
    ltoc(src -> lftSon, buff);
    buff += 3;

    for(i = 0; i < MAXKEYS; i++)
        if (i < src -> nodSiz) {
            memcpy(buff, src -> nodElmt[i].srchVal, len);
            buff += len;
            ltoc(src -> nodElmt[i].child, buff);
            buff += 3;
            ltoc(src -> nodElmt[i].spare, buff);
            buff += 3;
        }
    return (short) NULL;
}

/*
 *  item copy - copy one node item to another
 */
LOCAL void
itemcp(keyDes, dest, src)
register struct ITEM *dest, *src;
register short keyDes;
{
    (void) memcpy(dest -> srchVal, src -> srchVal, vindex[keyDes].keySize);
    dest -> child = src -> child;
    dest -> spare = src -> spare;
}

/*
 *    markFile - mark a file as being modified but not flushed.
 */
LOCAL short
markFile(keyDes)
register short keyDes;
{

    if (lseek(vindex[keyDes].fd, LNULL, SEEK_SET) == LERROR)
        return ERROR;
    if (read(vindex[keyDes].fd, (char *) &detail, sizeof(detail))
                                                    != sizeof(detail))
        return ERROR;
    detail.inUse = TRUE;
    if (lseek(vindex[keyDes].fd, LNULL, SEEK_SET) == LERROR)
        return ERROR;
    if (write(vindex[keyDes].fd, (char *) &detail, sizeof(detail)) !=
                                                            sizeof(detail))
        return ERROR;

    return (short) NULL;
}

/*
 *    node initializer - null out a new node
 */
LOCAL long
makNod(keyDes, ptr)
register short keyDes;
struct PAGE *ptr;
{
    register short i;
    char *pgPtr, *nodAlloc();

    pgPtr = nodAlloc((short)(MAXKEYS * (vindex[keyDes].keySize+1)));
    ptr -> nodSiz = (short) NULL;        /* no entries in this page        */
    ptr -> lftSon = LNULL;         /* leftmost leaf not allocated        */

    for(i=0; i < MAXKEYS; i++) {
        ptr -> nodElmt[i].srchVal = pgPtr + i * (vindex[keyDes].keySize+1);
        ptr -> nodElmt[i].child = LNULL;  /* no pages below this one    */
        ptr -> nodElmt[i].spare = LNULL;  /* no occurrances of item     */
    }
    return lioAvl(keyDes);         /* grab the next entry off free list  */
}

/*
 *    dynamically allocate space for a newly created or just read node.
 */
char *
nodAlloc(size)
short size;             /* number of bytes to allocate        */
{
    register short i;
    char *ptr;

    if ((ptr = malloc(size)) == (short) NULL) {
	puts("nodAllocErr");
	exit( (short) NULL);
    }

    (void) memset(ptr, (char) NULL, size);
    for(i=0; i<2*MAXTREEHEIGHT; i++) 
        if (allocPage[i].pgInUse==FALSE) 
            break;

    if (i == 2 * MAXTREEHEIGHT) {
	puts("nodAllocErr");
        exit(ERROR);
    }
    
    allocPage[i].pgInUse = TRUE;           /* deleted in one shot    */
    allocPage[i].pgPtr = ptr;              /* at end of the command  */
    return ptr;
}

/*
 *    free the pages allocated by the above function
 */
LOCAL void
nodFree(void)
{
    register short i;

    for(i = 0; i < 2*MAXTREEHEIGHT; i++) {
        if (allocPage[i].pgInUse) 
            free(allocPage[i].pgPtr);
        allocPage[i].pgInUse = FALSE;
        allocPage[i].pgPtr = NULL;
    }
}
/************************** End of B-Tree Clerical F'ns **********************/

/*
 *    btSearch for key keyValue on the b-tree with root subTrRoot.
 */
LOCAL short
btSearch(keyDes, subTrRoot, holdItem, subTrPtr)
short keyDes;
struct PAGE *subTrRoot;
struct ITEM *holdItem;
long subTrPtr;
{
    register short k, leftSide, rightBnd;
    short stackVal;
    register long son;
    char buffer[MAXKEYSIZE+1];
    struct ITEM currItem;
    struct PAGE temp;
    short tCmp;

    currItem.srchVal = buffer;

    if (subTrPtr == (short) NULL) {    /* prepare an item to be inserted   */
        adjFlag = TRUE;
        (void) memcpy(vindex[keyDes].matchName, keyValue,
                                                 vindex[keyDes].keySize);
        (void) memcpy(holdItem -> srchVal, keyValue, vindex[keyDes].keySize);
        holdItem -> child = LNULL;
        holdItem -> spare = keyRcdNo;
        if (amInserting) {
            vindex[keyDes].nKeys++;
            btResult=keyRcdNo;
        }
    }
    else {
	leftSide = (short) NULL;
        rightBnd = subTrRoot -> nodSiz - 1;
        do {
            k = (leftSide + rightBnd) / 2;
            tCmp = ndxCmp(keyDes, keyValue, subTrRoot -> nodElmt[k].srchVal);
	    if (tCmp <= (short) NULL)
                rightBnd = k - 1;
	    if (tCmp >= (short) NULL)
                leftSide = k + 1;
        } while(rightBnd >= leftSide);

	if (tCmp == (short) NULL) {
            btResult = subTrRoot -> nodElmt[k].spare;
            (void) memcpy(vindex[keyDes].matchName, keyValue,
                                                    vindex[keyDes].keySize);
            adjFlag = FALSE;
        }
        else {
            son = (ndxCmp(keyDes,keyValue,subTrRoot->nodElmt[0].srchVal) 
								< (short) NULL)
                   ? subTrRoot -> lftSon : subTrRoot -> nodElmt[rightBnd].child;
            if (son) {
                stackVal = vindex[keyDes].stackPtr;
                vindex[keyDes].stack[stackVal] = son;
                vindex[keyDes].stackPtr++;
                if (lioRdKey(keyDes, son, &temp)) 
                    return ERROR;
            }
            if (btSearch(keyDes, &temp, &currItem, son)) 
                return ERROR;

            if (adjFlag && amInserting) {
                if (btInsert(keyDes, subTrRoot, &currItem, rightBnd, holdItem))
                    return ERROR;
                if (lioWrtKey(keyDes, subTrPtr, subTrRoot)) 
                    return ERROR;
            }
            if (!amInserting) 
                adjFlag = FALSE;
        }
    }
    return (short) NULL;
}

/*
 *    insert the value keyValue into the b-tree
 */
LOCAL short
btInsert(keyDes, subTrRoot, thisItem, keyPositn, holdItem)
short keyDes, keyPositn;
struct PAGE *subTrRoot;
struct ITEM *thisItem, *holdItem;
{
    register long localPg;
    register short i;
    struct PAGE splitPg;

    if (subTrRoot -> nodSiz < MAXKEYS) {
        subTrRoot -> nodSiz++;
        adjFlag = FALSE;
        for(i = subTrRoot -> nodSiz - 1; i >= keyPositn + 2; i--)
            itemcp(keyDes, &subTrRoot -> nodElmt[i], 
                                            &subTrRoot -> nodElmt[i - 1]);
        itemcp(keyDes, &subTrRoot -> nodElmt[keyPositn + 1], thisItem);
    }
    else {
        hasOver = TRUE;
        if ((localPg = makNod(keyDes, &splitPg)) == LERROR) 
            return ERROR;
        if (keyPositn < MINKEYS) {
            if (keyPositn == MINKEYS - 1)
                itemcp(keyDes, holdItem, thisItem);
            else {
                itemcp(keyDes, holdItem, &subTrRoot -> nodElmt[MINKEYS - 1]);
                for(i = MINKEYS - 1; i >= keyPositn + 2; i--)
                itemcp(keyDes, &subTrRoot->nodElmt[i], 
                                                &subTrRoot->nodElmt[i-1]);
                itemcp(keyDes, &subTrRoot -> nodElmt[keyPositn + 1], thisItem);
            }
            for(i = 0; i < MINKEYS; i++)
                itemcp(keyDes,&splitPg.nodElmt[i],
                                                &subTrRoot->nodElmt[i+MINKEYS]);
        }
        else {
            keyPositn -= MINKEYS;
            itemcp(keyDes, holdItem, &subTrRoot -> nodElmt[MINKEYS]);
            for(i = 0; i < keyPositn; i++)
                itemcp(keyDes,&splitPg.nodElmt[i],
                                            &subTrRoot->nodElmt[i+MINKEYS+1]);
            itemcp(keyDes, &splitPg.nodElmt[keyPositn], thisItem);
            for(i = keyPositn + 1; i < MINKEYS; i++)
                itemcp(keyDes, &splitPg.nodElmt[i], 
                                            &subTrRoot->nodElmt[i+MINKEYS]);
        }
        subTrRoot -> nodSiz = splitPg.nodSiz = MINKEYS;
        splitPg.lftSon = holdItem -> child;
        holdItem -> child = localPg;
        if (lioWrtKey(keyDes, localPg, &splitPg)) 
            return ERROR;
    }
    return (short) NULL;
}

/*
 *   delete     search for and delete the key keyValue in a b-tree
 *              with root subTrRoot.
 */
LOCAL short
btDelete(keyDes, subTrRoot, subTrPtr)
short keyDes;
struct PAGE *subTrRoot;
long subTrPtr;
{
    register short i, k, leftSide, rightSide;
    struct PAGE temp;
    register long son;
    short tCmp;

    if (subTrPtr == (short) NULL)
        adjFlag = FALSE;
    else {
	leftSide = (short) NULL;
        rightSide = subTrRoot -> nodSiz - 1;
        do {
            k = (leftSide + rightSide) / 2;
            tCmp = ndxCmp(keyDes, keyValue, subTrRoot -> nodElmt[k].srchVal);
	    if (tCmp <= (short) NULL)
                rightSide = k - 1;
	    if (tCmp >= (short) NULL)
                leftSide = k + 1;
        } while (rightSide >= leftSide);

	son = (rightSide < (short) NULL) ? subTrRoot -> lftSon:
                                       subTrRoot -> nodElmt[rightSide].child;
	if (tCmp == (short) NULL) {
            btResult = subTrRoot -> nodElmt[k].spare;
            vindex[keyDes].nKeys--;
	    if (son == (short) NULL) {
                subTrRoot -> nodSiz--;
                adjFlag = subTrRoot -> nodSiz < MINKEYS;
                for(i = k; i < subTrRoot -> nodSiz; i++)
                    itemcp(keyDes,&subTrRoot->nodElmt[i],
                                                &subTrRoot->nodElmt[i+1]);
            }
            else {
                if (getRep(keyDes, son, subTrRoot, k)) 
                    return ERROR;
                if (adjFlag)
                    if (underflow(keyDes,subTrRoot,son,rightSide)) 
                        return ERROR;
            }
            if (lioWrtKey(keyDes, subTrPtr, subTrRoot)) 
                return ERROR;
        }
        else {
            if (son) if (lioRdKey(keyDes, son, &temp)) 
                return ERROR;
            if (btDelete(keyDes, &temp, son)) 
                return ERROR;

            if (adjFlag) {
                if (underflow(keyDes, subTrRoot, son, rightSide)) 
                    return ERROR;
                if (lioWrtKey(keyDes, subTrPtr, subTrRoot)) 
                    return ERROR;
            }
        }
    }
    return (short) NULL;
}

/*
 *    underflow - merge two adjacent pages if one underflows.
 */
LOCAL short
underflow(keyDes, ancestor, subTrRoot, srchPosn)
struct PAGE *ancestor;
long subTrRoot;
short srchPosn, keyDes;
{
    struct PAGE temp, brother;
    register short i, k, brthrSiz, ancstrSiz;
    register long b;

    if (lioRdKey(keyDes, subTrRoot, &temp)) 
        return ERROR;
    ancstrSiz = ancestor -> nodSiz;
    if (srchPosn + 1 < ancstrSiz) {
        srchPosn++;
        b = ancestor -> nodElmt[srchPosn].child;
        if (lioRdKey(keyDes, b, &brother)) 
            return ERROR;
        brthrSiz = brother.nodSiz;
        k = (brthrSiz - MINKEYS + 1) / 2;
        itemcp(keyDes, &temp.nodElmt[MINKEYS-1], 
                                        &ancestor -> nodElmt[srchPosn]);
        temp.nodElmt[MINKEYS - 1].child = brother.lftSon;

	if (k > (short) NULL) {
            for(i = 0; i < k - 1; i++)
                itemcp(keyDes, &temp.nodElmt[i + MINKEYS], &brother.nodElmt[i]);
            itemcp(keyDes, &ancestor -> nodElmt[srchPosn], 
                                                    &brother.nodElmt[k-1]);
            ancestor -> nodElmt[srchPosn].child = b;
            brother.lftSon = brother.nodElmt[k - 1].child;
            brthrSiz -= k;
            for(i = 0; i < brthrSiz; i++)
                itemcp(keyDes, &brother.nodElmt[i], &brother.nodElmt[i + k]);
            brother.nodSiz = brthrSiz;
            temp.nodSiz = MINKEYS - 1 + k;
            adjFlag = FALSE;
            if (lioWrtKey(keyDes, b, &brother)) 
                return ERROR;
        }
        else {
            for(i = 0; i < MINKEYS; i++)
                itemcp(keyDes, &temp.nodElmt[i + MINKEYS], &brother.nodElmt[i]);
            for(i = srchPosn; i < ancstrSiz - 1; i++)
                itemcp(keyDes, &ancestor -> nodElmt[i], 
                                                    &ancestor -> nodElmt[i+1]);
            temp.nodSiz = MAXKEYS;
            ancestor -> nodSiz = ancstrSiz - 1;
            if (lioFree(keyDes, b)) 
                return ERROR;
            adjFlag = ancestor -> nodSiz < MINKEYS;
        }
        if (lioWrtKey(keyDes, subTrRoot, &temp)) 
            return ERROR;
    }
    else {
	b = (srchPosn == (short) NULL) ? ancestor -> lftSon :
                                     ancestor -> nodElmt[srchPosn - 1].child;
        if (lioRdKey(keyDes, b, &brother)) 
            return ERROR;
        brthrSiz = brother.nodSiz + 1;
        k = (brthrSiz - MINKEYS) / 2;
	if (k > (short) NULL) {
	    for(i = MINKEYS - 2; i >= (short) NULL; i--)
            itemcp(keyDes, &temp.nodElmt[i + k], &temp.nodElmt[i]);
                itemcp(keyDes, &temp.nodElmt[k-1], 
                                                &ancestor -> nodElmt[srchPosn]);
            temp.nodElmt[k - 1].child = temp.lftSon;
            brthrSiz -= k;
	    for(i = k - 2; i >= (short) NULL; i--)
                itemcp(keyDes, &temp.nodElmt[i], &brother.nodElmt[i+brthrSiz]);
            temp.lftSon = brother.nodElmt[brthrSiz - 1].child;
            itemcp(keyDes,&ancestor->nodElmt[srchPosn],
                                                &brother.nodElmt[brthrSiz-1]);
            ancestor -> nodElmt[srchPosn].child = subTrRoot;
            brother.nodSiz = brthrSiz - 1;
            temp.nodSiz = MINKEYS - 1 + k;

            adjFlag = FALSE;
            if (lioWrtKey(keyDes, subTrRoot, &temp)) 
                return ERROR;
        }
        else {
            itemcp(keyDes, &brother.nodElmt[brthrSiz - 1],
                                                &ancestor->nodElmt[srchPosn]);
            brother.nodElmt[brthrSiz - 1].child = temp.lftSon;
            for(i = 0; i < MINKEYS - 1; i++)
                itemcp(keyDes, &brother.nodElmt[i+brthrSiz], &temp.nodElmt[i]);
            brother.nodSiz = MAXKEYS;
            ancestor -> nodSiz = ancstrSiz - 1;
            if (lioFree(keyDes, subTrRoot)) 
                return ERROR;
            adjFlag = ancestor -> nodSiz < MINKEYS;
        }
        if (lioWrtKey(keyDes, b, &brother)) 
            return ERROR;
    }
    return (short) NULL;
}

/*
 *    getRep - look for the best candidate to replace the item deleted.
 */
LOCAL short
getRep(keyDes, curNode, subTrRoot, k)
short keyDes, k;
long curNode;
struct PAGE *subTrRoot;
{
    struct PAGE temp;
    long son;

    if (lioRdKey(keyDes, curNode, &temp)) 
        return ERROR;
    son = temp.nodElmt[temp.nodSiz - 1].child;
    if (son) {
        if (getRep(keyDes, son, subTrRoot, k)) 
            return ERROR;
        if (adjFlag) {
            if (underflow(keyDes, &temp, son,  (short) (temp.nodSiz - 1))) 
                return ERROR;
            if (lioWrtKey(keyDes, curNode, &temp)) 
                return ERROR;
        }
    }
    else {
        temp.nodSiz--;
        temp.nodElmt[temp.nodSiz].child = subTrRoot -> nodElmt[k].child;
        itemcp(keyDes, &subTrRoot -> nodElmt[k], &temp.nodElmt[temp.nodSiz]);
        adjFlag = temp.nodSiz < MINKEYS;
        if (lioWrtKey(keyDes, curNode, &temp)) 
            return ERROR;
    }
    return (short) NULL;
}

/*
 *   simple entry point prolog
 */
LOCAL short
prolog(keyDes)
short keyDes;
{
    if (!initialized) 
        lioInit();
    if (lioSize(keyDes) == LNULL) 
        return ERROR;
    return (short) NULL;
}

/***************** End of all of the low level b-tree drivers ***************/

/*
 *  insert a new key into the tree
 */
LOCAL long trInsert(short keyDes, char *insKey, long insRcdNo)
{
    register long q;

    if (!initialized) 
        lioInit();
    hasOver = FALSE;
    btResult = NOTFOUND;
    (void)memcpy(keyValue, insKey, vindex[keyDes].keySize);
    if (vindex[keyDes].hasDup)
	(void) ltoc(LNULL, keyValue + vindex[keyDes].keySize - 3);
    keyRcdNo = insRcdNo;
    (void)memset((char *)vindex[keyDes].stack, (char) NULL,
					      sizeof(vindex[keyDes].stack));
    (void)memset(vindex[keyDes].matchName, (char) NULL,
					 sizeof(vindex[keyDes].matchName));
    vindex[keyDes].stack[0] = vindex[keyDes].treeRoot;
    vindex[keyDes].stackPtr = 1;

    if (vindex[keyDes].numRecs)
	if (lioRdKey(keyDes,vindex[keyDes].treeRoot,&rootPg))
	    return LERROR;
    if (btSearch(keyDes,&rootPg,&currItem,vindex[keyDes].treeRoot))
	return LERROR;

    if (adjFlag) {
	if (vindex[keyDes].numRecs)
	    if (lioWrtKey(keyDes, vindex[keyDes].treeRoot, &rootPg))
		return LERROR;
	q = vindex[keyDes].treeRoot;
        if ((vindex[keyDes].treeRoot=makNod(keyDes,&rootPg))==LERROR)
            return LERROR;
        rootPg.nodSiz = 1;
        rootPg.lftSon = q;
        itemcp(keyDes, &rootPg.nodElmt[0], &currItem);
        if (lioWrtKey(keyDes, vindex[keyDes].treeRoot, &rootPg)) 
            return LERROR;
    }
    nodFree();
    return btResult;
}

/*
 *  search  look for a key in the tree.
 *          85.05.21    fixed bug to do with search for a key after
 *                      first in a series of duplicates deleted.
 */
ENTRY long lioSearch(short keyDes, char *insKey)
{
    char tKey[MAXKEYSIZE];
    long recno, lioNext();

    if (prolog(keyDes))
	return LNULL;
    amInserting = FALSE;
    if (trInsert(keyDes, insKey, (long) NULL) == LERROR)
	return LERROR;
    if (btResult == LNULL && vindex[keyDes].hasDup) {
	if ((recno = lioNext(keyDes, tKey)) == LERROR)
	    return LERROR;
	if (!ndxCmp1(kkSize(keyDes), insKey, tKey))
	    return recno;
	else
	    return LNULL;
    }
    return btResult;
}

/*
 *  prepare to insert a new key
 */
ENTRY long
lioInsert(short keyDes, char *insKey, long insRcdNo)
{
    register long result;

    amInserting = TRUE;
    result = trInsert(keyDes, insKey, insRcdNo);
    amInserting = FALSE;
    /*
     *  this search is necessary to maintain the currency
     */
    if (trInsert(keyDes, insKey, (long) NULL) == LERROR) 
        return LERROR;
    return result;
}

/*
 *    delete - remove a key value from the index tree
 */
ENTRY long
lioDelete(keyDes, delKey)
short keyDes;
char *delKey;
{
    register long q;

    if (prolog(keyDes))
        return LNULL;
    btResult = NOTFOUND;

    if (delKey) {
        memcpy(keyValue, delKey, MAXKEYSIZE);
        if ( (q = lioSearch(keyDes, keyValue) ) <= (long) NULL)
            return q;
    }
    (void) memcpy(keyValue, vindex[keyDes].matchName, vindex[keyDes].keySize);
    (void) memset((char *)vindex[keyDes].stack, (char) NULL,sizeof(vindex[keyDes].stack));
    vindex[keyDes].stackPtr = (short) NULL;
    if (lioRdKey(keyDes, vindex[keyDes].treeRoot, &rootPg))
        return LERROR;
    if (btDelete(keyDes, &rootPg, vindex[keyDes].treeRoot))
        return LERROR;
    if (adjFlag)
        if (rootPg.nodSiz == 0) {
            q = vindex[keyDes].treeRoot;
            vindex[keyDes].treeRoot = rootPg.lftSon;
            if (lioFree(keyDes, q))
                return LERROR;
        }
    else
        if (lioWrtKey(keyDes, vindex[keyDes].treeRoot, &rootPg))
            return LERROR;

    nodFree();
    return btResult;
}

/*
 *    find the previous key in the tree
 */
ENTRY long
lioPrev(keyDes, foundKey)
register short keyDes;
char *foundKey;
{
    struct PAGE buf;
    register short i;
    short stackVal;
    long lioFirst();

    if (prolog(keyDes)) 
        return LNULL;

    if (lioRdKey(keyDes, vindex[keyDes].stack[vindex[keyDes].stackPtr-1], &buf))
        return LERROR;
    for(i = 0; i < buf.nodSiz; i++)
        if (ndxCmp(keyDes, buf.nodElmt[i].srchVal, vindex[keyDes].matchName)
								>= (short) NULL)
            break;

    if (buf.nodElmt[i].child == LNULL) {
	if (i != (short) NULL) {
            (void) memcpy(vindex[keyDes].matchName,
                            buf.nodElmt[i - 1].srchVal, vindex[keyDes].keySize);
            (void) memcpy(foundKey,
                            buf.nodElmt[i - 1].srchVal, kkSize(keyDes) );
            nodFree();
            return buf.nodElmt[i - 1].spare;
        }
        else {
            loop
		if (--vindex[keyDes].stackPtr == (short) NULL)
                    break;
                if (lioRdKey(keyDes, 
                            vindex[keyDes].stack[vindex[keyDes].stackPtr-1],&buf))
                    return LERROR;

		for(i = buf.nodSiz - 1; i >= (short) NULL; i--)
                    if (ndxCmp(keyDes,buf.nodElmt[i].srchVal,
				    vindex[keyDes].matchName) < (short) NULL) {
                        (void) memcpy(vindex[keyDes].matchName,
                        buf.nodElmt[i].srchVal, vindex[keyDes].keySize);
                        (void) memcpy(foundKey, buf.nodElmt[i].srchVal,
                                                                kkSize(keyDes) );
                        nodFree();
                        return buf.nodElmt[i].spare;
                    }
            endLoop
            if (lioFirst(keyDes, foundKey) < LNULL) 
                return LERROR;
            nodFree();
            return btResult;
        }
    }
    else {
        stackVal = vindex[keyDes].stackPtr;
        vindex[keyDes].stack[stackVal] =
		    (i != (short) NULL) ? buf.nodElmt[i-1].child : buf.lftSon;
        vindex[keyDes].stackPtr++;
        loop
            if (lioRdKey(keyDes,vindex[keyDes].stack[vindex[keyDes].stackPtr-1],
                                                    &buf))
                return LERROR;
            stackVal = vindex[keyDes].stackPtr;
            if ((vindex[keyDes].stack[stackVal]=
			    buf.nodElmt[buf.nodSiz - 1].child) == (short) NULL)
                break;
            vindex[keyDes].stackPtr++;
        endLoop
        (void) memcpy(vindex[keyDes].matchName,
                     buf.nodElmt[buf.nodSiz-1].srchVal, vindex[keyDes].keySize);
        (void) memcpy(foundKey,
                           buf.nodElmt[buf.nodSiz-1].srchVal, kkSize(keyDes) );
        nodFree();
        return buf.nodElmt[buf.nodSiz - 1].spare;
    }
}

/*
 *  Search Partial  Do a search looking for the key that is greater
 *                  than or equal to the search key. If the end of
 *                  the tree is hit, return the last key in the tree.
 */
ENTRY long
lioSrPrtl(keyDes, srchKey, foundKey)
short keyDes;
char *srchKey, *foundKey;
{
    struct PAGE srchPg;
    register short i;
    short stackVal;
    long save, lioLast();

    if (prolog(keyDes)) 
        return LNULL;

    (void) trInsert(keyDes, srchKey, (long) NULL);
    /*
     *  we have the key before the one greater than or equal to the search
     *  key. Look for the next one in the tree.
     */
    do {
        if (lioRdKey(keyDes, vindex[keyDes].stack[vindex[keyDes].stackPtr-1],
                                                        &srchPg)) 
            return LERROR;
        for(i = 0; i < srchPg.nodSiz; i++)
            if (ndxCmp1(kkSize(keyDes), srchKey, srchPg.nodElmt[i].srchVal) <=
							    (short) NULL) {
                (void) memcpy(vindex[keyDes].matchName, 
                            srchPg.nodElmt[i].srchVal, vindex[keyDes].keySize);
                (void) memcpy(foundKey, srchPg.nodElmt[i].srchVal, 
                                                            kkSize(keyDes) );
                nodFree();
                return srchPg.nodElmt[i].spare;
            }

        stackVal = vindex[keyDes].stackPtr--;
	vindex[keyDes].stack[stackVal] = (short) NULL;
    } while(vindex[keyDes].stackPtr > (short) NULL);

    /*
     *   looking for key > largest key in tree, return largest key.
     */
    if ( (save = lioLast(keyDes, foundKey)) < LNULL)
        return LERROR;
    (void) memcpy(vindex[keyDes].matchName, foundKey, kkSize(keyDes) );

    nodFree();
    return save;
}

/*
 *    get next key - return the key just greater than the one previously
 *           returned by a search or searchPartial call.
 */
ENTRY long lioNext(short keyDes, char *foundKey)
{
    struct PAGE buf;
    register short i;
    short stackVal;
    long lioLast();

    if (prolog(keyDes))
	return LNULL;
    if (lioRdKey(keyDes, vindex[keyDes].stack[vindex[keyDes].stackPtr-1], &buf))
	return LERROR;
    for(i = buf.nodSiz - 1; i >= (short) NULL; i--)
    if (ndxCmp(keyDes, buf.nodElmt[i].srchVal, vindex[keyDes].matchName)
								<= (short) NULL)
	break;

    if (buf.nodElmt[i].child == LNULL) {
	if (i < buf.nodSiz - 1) {
	    (void) memcpy(vindex[keyDes].matchName,
			    buf.nodElmt[i + 1].srchVal, vindex[keyDes].keySize);
	    (void) memcpy(foundKey,
				buf.nodElmt[i + 1].srchVal, kkSize(keyDes) );
	    nodFree();
	    return buf.nodElmt[i + 1].spare;
	}
	else {
	    loop
		if (--vindex[keyDes].stackPtr == (short) NULL)
		    break;
		if (lioRdKey(keyDes,
			    vindex[keyDes].stack[vindex[keyDes].stackPtr-1],&buf))
		    return LERROR;
		for(i = 0; i < buf.nodSiz; i++)
		    if (ndxCmp(keyDes,buf.nodElmt[i].srchVal,
				    vindex[keyDes].matchName) > (short) NULL) {
			(void) memcpy(vindex[keyDes].matchName,
				 buf.nodElmt[i].srchVal, vindex[keyDes].keySize);
			(void) memcpy(foundKey, buf.nodElmt[i].srchVal,
							    kkSize(keyDes) );
			nodFree();
			return buf.nodElmt[i].spare;
		    }
	    endLoop
	    if (lioLast(keyDes, foundKey) < LNULL)
		return LERROR;
	    nodFree();
	    return btResult;
	}
    }
    else {
        stackVal = vindex[keyDes].stackPtr;
        vindex[keyDes].stack[stackVal] = buf.nodElmt[i].child;
        vindex[keyDes].stackPtr++;
        loop
            if (lioRdKey(keyDes,vindex[keyDes].stack[vindex[keyDes].stackPtr-1],&buf))
                return LERROR;
            stackVal = vindex[keyDes].stackPtr;
	    if ((vindex[keyDes].stack[stackVal] = buf.lftSon) == (short) NULL)
                break;
            vindex[keyDes].stackPtr++;
        endLoop
        (void) memcpy(vindex[keyDes].matchName, buf.nodElmt[0].srchVal, vindex[keyDes].keySize);
        (void) memcpy(foundKey, buf.nodElmt[0].srchVal, kkSize(keyDes) );
        nodFree();
        return buf.nodElmt[0].spare;
    }
}

/*
 *   first - find the first key in a tree
 */
ENTRY long
lioFirst(keyDes, fk)
short keyDes;
char *fk;
{
    if (prolog(keyDes)) 
        return LNULL;
    (void) memset((char *) vindex[keyDes].stack, (char) NULL,sizeof(vindex[keyDes].stack));
    (void) memset(vindex[keyDes].matchName, (char) NULL, vindex[keyDes].keySize);
    vindex[keyDes].stackPtr = (short) NULL;
    btResult = NOTFOUND;
    if (btFirst(keyDes, vindex[keyDes].treeRoot, fk)) 
        return LERROR;
    nodFree();
    return btResult;
}

/*
 *   descend the leftmost branches of a tree looking for the smallest leaf.
 */
LOCAL short
btFirst(keyDes, rootNod, fk)
short keyDes;
long rootNod;
char *fk;
{
    register long son;
    short stackVal;
    struct PAGE nextPage;

    if (rootNod != LNULL) {
        if (lioRdKey(keyDes, rootNod, &nextPage)) 
            return ERROR;
        son = nextPage.lftSon;
        stackVal = vindex[keyDes].stackPtr;
        vindex[keyDes].stack[stackVal] = rootNod;
        vindex[keyDes].stackPtr++;
        if (son) {
            if (btFirst(keyDes, son, fk)) 
                return ERROR;
        }
        else {
            (void) memcpy(vindex[keyDes].matchName, nextPage.nodElmt[0].srchVal,
                                                        vindex[keyDes].keySize);
            (void) memcpy(fk, nextPage.nodElmt[0].srchVal, kkSize(keyDes) ) ;
            btResult = nextPage.nodElmt[0].spare;
        }
    }
    return (short) NULL;
}

/*
 *   last - find the last key in a tree
 */
ENTRY long
lioLast(keyDes, fk)
short keyDes;
char *fk;
{
    if (prolog(keyDes)) 
        return LNULL;
    (void) memset((char *) vindex[keyDes].stack, (char) NULL,sizeof(vindex[keyDes].stack));
    (void) memset(vindex[keyDes].matchName, (char) NULL, vindex[keyDes].keySize);
    vindex[keyDes].stackPtr = (short) NULL;
    btResult = NOTFOUND;
    if (btLast(keyDes, vindex[keyDes].treeRoot, fk)) 
        return LERROR;
    nodFree();
    return btResult;
}

/*
 *   descend the rightmost branches of a tree looking for the largest leaf.
 */
LOCAL short
btLast(keyDes, rootNod, fk)
short keyDes;
long rootNod;
char *fk;
{
    register long son;
    short stackVal;
    struct PAGE nextPage;

    if (rootNod != LNULL) {
        if (lioRdKey(keyDes, rootNod, &nextPage)) 
            return ERROR;
        son = nextPage.nodElmt[nextPage.nodSiz-1].child;
        stackVal = vindex[keyDes].stackPtr;
        vindex[keyDes].stack[stackVal] = rootNod;
        vindex[keyDes].stackPtr++;
        if (son) {
            if (btLast(keyDes, son, fk)) 
                return ERROR;
        }
        else {
            (void) memcpy(vindex[keyDes].matchName, 
                        nextPage.nodElmt[nextPage.nodSiz-1].srchVal,
                            vindex[keyDes].keySize);
            (void) memcpy(fk, nextPage.nodElmt[nextPage.nodSiz-1].srchVal,
                            kkSize(keyDes) );
            btResult = nextPage.nodElmt[nextPage.nodSiz-1].spare;
        }
    }
    return (short) NULL;
}

/*
 *  find the key with a specific data record number. Return the record
 *  number if found, LNULL otherwise.
 */
ENTRY long
lioSerNum(keyDes, key, num)
short keyDes;
char *key;
long num;
{
    char buf[MAXKEYSIZE+1];
    long found, prev, lioSearch(), lioNext();

    if ( (prev = found = lioSearch(keyDes, key)) == LERROR)
        return LNULL;

    if ( !vindex[keyDes].hasDup)
        return (found == num) ? found : LNULL;

    loop
        if (found == num)
            return num;
        if ( (found = lioNext(keyDes, buf)) == LERROR)
            return LNULL;
	if ( ndxCmp1( kkSize(keyDes), buf, key) != (short) NULL)
            break;
        if ( found == prev )
            break;
        prev = found;
    endLoop

    return LNULL;
}

/*
 *  comment this include out if treeprint is not needed'
 *  Only people doing fairly heavy mods to the actual btree routines
 *  should need this.
 */
/* #include "lioTrPr.c" */

/* ------------------------- Modifiable functions ------------------------- */

/*
 *   Index compare
 *   - this function compares two values (Left & Right) and returns:
 *  1 if L > R
 *  0 if L == R and
 *     -1 if L < R
 *
 *   If you want to use 'custom' keys, such as integer or float
 *   change this function.
 */
LOCAL short
ndxCmp(keyDes, left, right)
short keyDes;
char *left, *right;
{
   short i;
   long l, r;

   i = ndxCmp1(vindex[keyDes].keySize, left, right);
   if (!vindex[keyDes].hasDup || i) return i;

   i = vindex[keyDes].keySize - 3;
   if (amInserting && (ctol(left + i) == (long) NULL)) {
      (void) ltoc(++vindex[keyDes].dupNum, left + vindex[keyDes].keySize - 3);
      return 1;
   }

   l = ctol(left + i);
   r = ctol(right + i);
   if (l < r) return -1;
   if (l > r) return 1;

   return (short) NULL;
}

/*
 *  compare the two strings given for a maximum of <len> bytes
 */
LOCAL short
ndxCmp1(len, left, right)
short len;
char *left, *right;
{
   short i, temp;

/*
   i = memcmp((void *)left, (void *)right, len);
   return i;
*/

	for(i = 0; i < len; i++) {

		if ((unsigned char)left[i] == 0xFF &&
		    (unsigned char)left[i+1] == 0xFF)
		    return (short)NULL;

#ifdef SIGN_EXT
		if ((temp = rmsign(left[i]) - rmsign(right[i]) ) != 0)
#else
		if ((temp = (short) left[i] - (short) right[i]) != 0)
#endif
			return temp;
		/*
		if (left[i] == (short) NULL)
			return (short) NULL;
		*/

	}
	return (short) NULL;

}

