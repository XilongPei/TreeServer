/*
 *  btgconst.h      Header file for btree.c and isam.c
 *
 *
 *              There are detailed comments above all the paramaters that
 *              can or should be tailored. Please take a few minutes and
 *              read through this file; all troubles getting the package
 *              working initially are due to one or more of these #defines
 *              being incorrectly set.
 *
 *                  fcntl.h         Some compilers require this header be
 *                                  #included for file access routines. 
 *                                  If yours is not one of them, comment out
 *                                  the #include.
 *
 *                  CREATMODE       The argument passed to creat()
 *
 *                  RWMODE          The argument passed to open()
 *          
 *                  voids           If your compiler does NOT support voids,
 *                                  uncomment the 'void' #define below.
 *
 *                  Sign Extension  It will work for all compilers as set;
 *                                  if you know that your compiler treats
 *                                  char's as unisgned integers, then
 *                                  commenting out the #define can result in
 *                                  a small speed increase.
 *
 *                  MAXKEYSIZE      The maximum key length. Don't
 *                                  forget the trailing NULL and the
 *                                  extra 3 byte overhead per key for
 *                                  duplicates.
 *
 *                  NUMINDEXS       The maximum number of index files that
 *                                  can be open at one time.
 *                  
 */

#ifndef __BTREE_H
#define __BTREE_H
/*
 *  fcntl.h
 *
 *  Some compilers have a header file that must be included for file
 *  input/output functions. If your compiler comes with a fcntl.h file,
 *  then uncomment this #include
 */
#include <fcntl.h>
#include <sys\\stat.h>

/*
 *  Some common CREATMODE and RWMODE values
 *  ---- ------ --------- --- ------ ------
 *
 *  CWare DeSmet C      CREATMODE  == 0644
 *                      RWMODE     == 2
 *                      (sign extends chars)
 *
 *  Wizard C            CREATMODE  == 0200      [see below for Wizard notes]
 *                      RWMODE     == O_RDWR
 *
 *  MicroSoft v3.x      CREATMODE  == 0200
 *                      RWMODE     == (O_RDWR | O_BINARY)
 *                      (sign extends chars)
 *
 *  Hi-Tech C           CREATMODE  == 0200   (not really critical)
 *                      RWMODE     == 2
 *                      (doesn't require fcntl.h #include)
 *
 *  Lattice < 3.0       CREATMODE  == 0x8644
 *                      RWMODE     == 0x8002
 *
 *  Lattice 3.0+        CREATMODE == (0x8002 | S_IWRITE | S_IREAD)
 *                      RWMODE    == 0x8002
 *
 *  C.I. C86            CREATMODE  == BUPDATE
 *                      RWMODE     == BUPDATE
 *
 *  unix/xenix          CREATMODE  == 0644
 *                      RWMODE     == 2
 */
#ifdef 	LATTICE
#define CREATMODE    	(O_RAW | O_RDWR | S_IWRITE | S_IREAD)
#define RWMODE       	(O_RAW | O_RDWR )
#endif

#ifdef 	DESMET
#define CREATMODE	0644
#define RWMODE       	2
#endif

#ifdef 	MSC
#define CREATMODE    	(0200 | O_BINARY)
#define RWMODE       	(O_BINARY | O_RDWR )
#endif

/*

#ifdef __TURBOC__
#define    CREATMODE    ( O_CREAT | O_TRUNC | O_BINARY | S_IWRITE | S_IREAD)
#define    RWMODE       (O_BINARY | O_RDWR )
#endif
*/
/*
*	When creating a binary file in Borland C++ 3.1 , this mode cause
*
*
*	modified by Marlin 9,21,1994
*/
/*

#ifdef 	__TURBOC__
#define CREATMODE    	(O_CREAT | O_WRONLY | O_BINARY | S_IWRITE | S_IREAD)
#define RWMODE       	(O_BINARY | O_RDWR )
#endif
*/

#ifdef __TURBOC__
#define    CREATMODE    (O_CREAT | O_TRUNC | O_BINARY), (S_IREAD|S_IWRITE)
#define    RWMODE       (O_BINARY | O_RDWR )
#endif

/*
#ifndef CREATMODE
#define CREATMODE 	0644
#define RWMODE       	2
#endif
*/
#define    CREATMODE    (O_CREAT | O_TRUNC | O_BINARY), (S_IREAD|S_IWRITE)
#define    RWMODE       (O_BINARY | O_RDWR )

#ifndef	BLOCKSIZE
#define BLOCKSIZE	512
#endif

/*
 *  Wizard C Users: Please read this!
 *
 *  Currently there is no way to creat() a binary files using Wizard.
 *  Although a fix is on the horizon (it's easy to do; phone them for
 *  details), the simplest approach is to uncomment the following two
 *  #defines and everything will work beautifly.
 */

/*
#define read(a, b, c) _read(a, b, c)
#define write(a, b, c) _write(a, b, c)
*/

/*
 *  Voids
 *  -----
 *  if your compiler supports void types, comment out the void #define.
 */
/*  #define void short */

/* Remed by NiuJingyu
#ifndef void
#define VOID
#endif
/*

/*
 *  the horror of SIGN EXTENSION
 *
 *  Most compilers we've played with treat characters as unsigned (that
 *  is in the range from 0 to 255). Some compilers sign extend, which can have
 *  some nasty effects. To determine if your compiler is one of the
 *  offenders, set TIMES (in driver.c) to 300. If it fails at 255, uncomment
 *  the define below.
 *
 *  Leaving this define in will work for all compilers at the cost of a (very)
 *  slight slow-down.
 *
 */
#define    SIGN_EXT

/*
 *  other values that can be tailored
 */

//special define for TGMIS ^-_-^ 1996.3.12
#define    MAXKEYSIZE    32    /* max # of chars in key (less null)  */
#define    NUMINDEXS     4    /* max # of open files (key and data) */

#define    MINKEYS       29    /* b-tree order                       */
#define	   MAXSHORTNUM	 4	   /* add by marlin */
/*
 *  end of stuff that should be changed
 */

/*
 *      General purpose Symbolic constants:
 */
#define OK 0            /* General purpose "no error" return value          */
#define SECSIZ 128      /* Sector size for header read/write calls          */
#define LOCAL static    /* f'ns local to this module                        */
#define ENTRY           /* entry points into this module                    */

//special define for TGMIS ^-_-^ 1996.3.12
#define MAXTREEHEIGHT 6   /*OLD: huge height 10 tree (10^10 keys)              */

/*
 *  structure for lioGet/PutCur(). These are currently undocumented
 *  entry points that set and restore BTree currency.
 */
struct currency {
	short sPtr;
        long stk[MAXTREEHEIGHT];
        char last[MAXKEYSIZE];
    };

/*
 *  #defines that may already be defined in stdio.h
 */

/*
 *  NULL is used as a non-success return code
 */
#ifndef NULL
#define NULL 0
#endif

/*
 *  many BTree routines need a (long) NULL
 */
#ifndef LNULL
#define LNULL (long) NULL
#endif

/*
 *  file position equates for btFPos() (only used in isam)
 */
#ifndef EOF
#define EOF (-1)        /* end of file       */
#endif
#define BOF (-2)        /* beginning of file */
#define WHOKNOWS (-3)    /* unknown position  */

/*
 *  general "everything's OK" value
 */
#ifndef GOOD
#define GOOD 1
#endif

/*
 *  life threatening error value
 */
#ifndef ERROR
#define ERROR -1
#endif

/*
 *  true/false
 */
#ifndef TRUE
#define TRUE 1 
#define FALSE 0
#endif

/*
 *        the t(ype) codes of a key value     (currently only string supported)
 */
#define    tSTRING       1             /* key is a string                   */
#define    tDECIMAL      2             /* key is floating point decimal     */
#define    tINTEGER      3             /* key is signed 16 bit integer      */
#define    tLONG         4             /* key is signed 32 bit integer      */
#define    tSHORT        5             /* key size is only 8 bit integer    */
#define    tDATA         6             /* file is actually data, not keys   */

/*
 *  (rarely needed) defines for the treeprint routine
 */
#define    NOTFOUND     0
#define    PREORDER     1              /* do an pre-order tree print        */
#define    INORDER      2              /* do an in-order tree print         */


/*
 *    general useful constants, but only used in this file.
 */
#define    MAXKEYS  2*MINKEYS      /* b-tree page size          */
#define    loop     while (TRUE) {
#define    endLoop  }
#define    LERROR   -1L
#define    NUMFIELDS    10         /* max number of fields in key - NOT USED */
#define    FILEXIST NULL           /* access mode for checking file pres     */
#define    NOCREAT  LNULL          /* creat returns 0 on error in unix       */
#define    NOPEN    ERROR          /* open returns -1 on error in unix       */
#define    NAMESIZE 20             /* number of chars that make up a         */
                                   /* file (path) name                       */
/*
 *   each page entry has the following attributes
 */
struct ITEM {       /* definition of a list of entries           */
   char *srchVal;   /* each entry has a key                       */
   long spare;      /* the data file record number                */
   long child;      /* a pointer to entries > key and < key + 1   */
};

/*
 *    the B-tree node structure
 */
struct PAGE {
   short nodSiz;          /* number of items in this node        */
   long lftSon;           /* pointer to leftmost subpage         */
   struct ITEM nodElmt[MAXKEYS];  /* list of keys & page pointers  */
};

/*
 *    this is what a file header block looks like.
 */

    struct btTail {
        char lioName[NAMESIZE]; /* name of the file                          */
        short order;            /* the order of the b-tree                   */
        short kType;            /* keys of different types supported?        */
	short kSize;            /* size of the key field (in bytes)          */
	long freeNum;           /* log-rec # for first entry free l-list     */
	short pageSize;           /* the number of bytes in each page rec.     */
	long rootNum;           /* logical record number of the root page    */
	long numKeys;           /* how many keys currently allocated         */
	long numRcds;           /* how many pages currently allocated        */
	short inUse;            /* set on file open, reset on close          */
	struct {                /* NOT USED AT THIS TIME                     */
	    short fieldNo;
	    short startCh;
	    short endCh;
	} keyForm[NUMFIELDS];
	short dupOk;            /* are duplicates allowed on this index?     */
	long dupVal;            /* unique id assigned to a duplicate insert  */
	char fill[BLOCKSIZE - 112];	/*add by marlin 9,21,1994 just want to make the head to be a block */
    } ;


LOCAL struct indexTag  { /* global variables required to maintain the key    */
   short keySize;          /* files on a data file.                            */
   short keyType;
   short keyOrder;
   short keyPgSz;
   int   fd;

   short trInUse;
   long numRecs;                    /* how many pages currently allocated   */
   long nKeys;                      /* how many keys r currently allocated  */
   long nextAvl;                    /* next logical record on the free list */
   long treeRoot;                   /* logical record number of root node   */
   short stackPtr;                  /* record # path to last page searched  */
   long stack[MAXTREEHEIGHT];       /* remember path for sequential searchs */
   char matchName[MAXKEYSIZE+1];    /* last key matched by sequential search*/
   long lastRcd;                    /* record number of last record found   */
   short hasDup;                    /* duplicates allowed on this index?    */
   long dupNum;                     /* next available duplicate number      */
   char fName[NAMESIZE];            /* name the file was opened with         */

};

/*
 *  cache details
 *
 *  Our tests here (on an 8mhz 266 MS-DOS machine) indicate that 5 is
 *  a near-optimum number of slots; you may wish to experiment with your
 *  own flavour of hardware.
 *
 *  Setting MAXCACHE to 1 effectively disables all cache operations.
 */

/*
#define MAXCACHE 5
LOCAL struct btCache {
    short cfd;
    long cRecNum;
    short dirty;
    unsigned lru;
    char temp[ (MAXKEYS*(MAXKEYSIZE+6)) + 3 + sizeof(char)];
};
*/
//modified by marlin 9,21,1994
#define MAXCACHE 5
LOCAL struct btCache {
    short cfd;
    long cRecNum;
    short dirty;
    unsigned lru;

    short cacheSize;	/*indicat the cache buffer size*/
    char *temp;
};

/* ------------------------------------------------------------------------ */

/*
 *  initialize the in memory tables. Remains an entry point for compatibility.
 */
ENTRY void lioInit( void );

/*
 *  The following two routines are used to save and restore the BTree
 *  currency. The structure 'currency' is defined in btgconst.h.
 *
 *  copy out the current currency values
 */
ENTRY void lioGetCur(short keyDes, struct currency *p);

/*
 *   replace the currency information
 */
ENTRY void lioPutCur(short keyDes, struct currency *p);

/*
 *    return the number of keys/records for a data or index file
 */
ENTRY long lioSize(register short keyDes);

/*
 *   return the size of a 'unit' of storage.
 */
ENTRY short lioPSize(register short keyDes);

/*
 *   return a boolean that says whether duplicates are allowed
 */
ENTRY short lioDup(short keyDes);

/*
 *   get the logical record number of the next free record
 */
ENTRY long lioAvl(register short keyDes);

/*
 *  close an index file
 */
ENTRY short lioClose(register short keyDes);

/*
 *  flush an index or data file
 */
ENTRY short lioFlush(short keyDes);

/*
 *    Initialize a new index file.
 */
ENTRY short lioCreat(char *fName, short keySiz, short keyTyp, char dupAllowed);

/*
 *  return an allocated logical record number to the free list
 */
ENTRY short lioFree(short keyDes, long discard);

/*
 *  lioOpen - open an existing key or data file.
 */
ENTRY short lioOpen(char *fName);

/*
 *  get a logical page from the file fd
 */
ENTRY short lioRead(register short keyDes, register long pageNo, char *dest);

/*
 *  put a logical page to the file fd
 */
ENTRY short lioWrite(register short keyDes, register long pageNo, char *src);

/*
 *  kkSize  Return the size of the actual index keysize MINUS any
 *          duplicate key bytes.
 */
ENTRY short lioKsize(short btDes);

/*
 *  rdCache     Return pointer to the requested page. Update
 *              the cache if necessary.
 */
LOCAL char *rdCache(short keyDes, long pageNo);

/*
 *  wrtCache        Return a pointer to where the given page should be written.
 *                  Flush the cache as necessary.
 */
LOCAL char *wrtCache(short keyDes, long pageNo);

/*
 *  flCache     Flush all dirty buffers to disk. Reset lru to the
 *              beginning. Clear the cache.
 */
LOCAL short flCache(void);

/*
 *  lioReadKey  read a page from the disk, get space for all active
 *              keys and install pointers to them in the node.
 */
LOCAL short lioRdKey(short keyDes, long pageNo, struct PAGE *dest);

/*
 *  lioWriteKey     write a page to the disk, rebuild the node before 
 *                  writing.
 */
LOCAL short lioWrtKey(short keyDes, long pageNo, struct PAGE *src);

/*
 *  item copy - copy one node item to another
 */
LOCAL void itemcp(register short keyDes, register struct ITEM *dest, \
		  register struct ITEM *src);

/*
 *    markFile - mark a file as being modified but not flushed.
 */
LOCAL short markFile(register short keyDes);

/*
 *    node initializer - null out a new node
 */
LOCAL long makNod(register short keyDes, struct PAGE *ptr);

/*
 *    dynamically allocate space for a newly created or just read node.
 */
char *nodAlloc(short size);

/*
 *    free the pages allocated by the above function
 */
LOCAL void nodFree(void);

/************************** End of B-Tree Clerical F'ns **********************/


/*
 *    btSearch for key keyValue on the b-tree with root subTrRoot.
 */
LOCAL short btSearch(short keyDes, struct PAGE *subTrRoot, \
		   struct ITEM *holdItem, long subTrPtr);

/*
 *    insert the value keyValue into the b-tree
 */
LOCAL short btInsert(short keyDes, struct PAGE *subTrRoot, \
		   struct ITEM *thisItem, short keyPositn, \
		   struct ITEM *holdItem);

/*
 *   delete     search for and delete the key keyValue in a b-tree
 *              with root subTrRoot.
 */
LOCAL short btDelete(short keyDes, struct PAGE *subTrRoot, long subTrPtr);

/*
 *    underflow - merge two adjacent pages if one underflows.
 */
LOCAL short underflow(short keyDes, struct PAGE *ancestor, \
		    long subTrRoot, short srchPosn);

/*
 *    getRep - look for the best candidate to replace the item deleted.
 */
LOCAL short getRep(short keyDes, long curNode, struct PAGE *subTrRoot, short k);

/*
 *   simple entry point prolog
 */
LOCAL short prolog(short keyDes);

/***************** End of all of the low level b-tree drivers ***************/

/*
 *  insert a new key into the tree
 */
LOCAL long trInsert(short keyDes, char *insKey, long insRcdNo);

/*
 *  search  look for a key in the tree.
 *          85.05.21    fixed bug to do with search for a key after
 *                      first in a series of duplicates deleted.
 */
ENTRY long lioSearch(short keyDes, char *insKey);

/*
 *  prepare to insert a new key
 */
ENTRY long lioInsert(short keyDes, char *insKey, long insRcdNo);

/*
 *    delete - remove a key value from the index tree
 */
ENTRY long lioDelete(short keyDes, char *delKey);

/*
 *    find the previous key in the tree
 */
ENTRY long lioPrev(register short keyDes, char *foundKey);

/*
 *  Search Partial  Do a search looking for the key that is greater
 *                  than or equal to the search key. If the end of
 *                  the tree is hit, return the last key in the tree.
 */
ENTRY long lioSrPrtl(short keyDes, char *srchKey, char *foundKey);

/*
 *    get next key - return the key just greater than the one previously
 *           	     returned by a search or searchPartial call.
 */
ENTRY long lioNext(short keyDes, char *foundKey);

/*
 *   first - find the first key in a tree
 */
ENTRY long lioFirst(short keyDes, char *fk);

/*
 *   descend the leftmost branches of a tree looking for the smallest leaf.
 */
LOCAL short btFirst(short keyDes, long rootNod, char *fk);

/*
 *   last - find the last key in a tree
 */
ENTRY long lioLast(short keyDes, char *fk);

/*
 *   descend the rightmost branches of a tree looking for the largest leaf.
 */
LOCAL short btLast(short keyDes, long rootNod, char *fk);

/*
 *  find the key with a specific data record number. Return the record
 *  number if found, LNULL otherwise.
 */
ENTRY long lioSerNum(short keyDes, char *key, long num);

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
LOCAL short ndxCmp(short keyDes, char *left, char *right);

/*
 *  compare the two strings given for a maximum of <len> bytes
 */
LOCAL short ndxCmp1(short len, char *left, char *right);

#endif
