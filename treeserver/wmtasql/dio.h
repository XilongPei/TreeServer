/***************************************************************************\
 * FILENAME:                    dio.h
 * Copyright: (c)
 *     Shanghai Tiedao University 1990-1997
 *     East-Union Computer Service Co., Ltd. 1994
 *     China Railway Software Company. 1996-1997
 *     Shanghai Withub Vision Software Co., Ltd. 1999-2000
 *
 * MainAuthor:
 *     Xilong Pei
 * Statements:
 *
\***************************************************************************/


/*==========
 * dERROR =     1012: name is too long
 *              1003: file not found
 *              1001: ENOMEM
 *              1013: buffer is too little
 *              2000: DBF is readonly, and set the dFILE_OFLAG
 *              2001: DBT is readonly, and set the dFILE_OFLAG
 *              3001: Field_num is bigger than the df->field_num
 *              4001: DBT file can't be opened
 *              4002: DBT Record is too long( longer than 10 blocks)
 *==========
 */

#ifndef _IncludedDIOProgramHead_
#define _IncludedDIOProgramHead_        "DIO v1.00"

//for WindowsNT Informix use


#include <windows.h>    // This header file include for ESQL/C use.
#include <process.h>
#include "sqlda.h"
#include "sqlca.h"
#include "wst2mt.h"

#ifdef RJ_SUPPORT
    #include "rjio.h"
#endif

#define NAMELEN         260
#define FIELDNAMELEN    32

#define EXTLEN          4
#define FIELDMAXLEN	256
#define dMAXDECLEN      256       /* the min is 10 ! */
#define dIOMaxUseRecord 32000     /* the max value of dIO use a record, it */
				  /*   should less than 0B1111111=127, change this:1998.1.2 Xilong  */
#define DBTDELMEMORYSIZE        DBTBLOCKSIZE / 4
#define DIO_MAX_SYNC_BTREE	10

#define DBTBLOCKSIZE            512
#define DBT_DATASIZE            (512-8)
#define dBITMEMOMemoMark        0x004E
#define SYSSIZE         4096    /* FOXBASE & dBASE3 rec_len < 4000 byte */
#define dMINDBTSIZE	4096

#ifdef WSToMT
#define _DIOFILENUM_    100
#else
#define _DIOFILENUM_    20
#endif

#define TABLENAMELEN    20
#define CURSORNAMELEN   10
#define IDLEN		10


//this value is relation to MAX_KEYFIELD_NUM which is defined in btree.h
//they have the same value
#define dioMAX_KEYFIELD_NUM	5
//				MAX_KEYFIELD_NUM


#define VIEWFLAG        1
#define TABLEFLAG       2

#define dKEY_UNIQUE	1
#define dKEY_FREE	2

#define VIEW_RECNO_FIELD        "R_E_C_N_O_"
#define MAX_LINE_LENGTH		4096
#define MAX_ASQL_SCRIPT_LEN	65534

#define dDBTTMPFILEEXT	"._BT"

#ifndef __dFIELD_
#define __dFIELD_
typedef struct {
	unsigned char field[FIELDNAMELEN];
	unsigned char fieldtype;
	long 	      fieldlen;
	unsigned char fielddec;
	long	      sdword;		//use for ODBC data, 1998.7.1
	void          *px;
	unsigned char *fieldstart;
} dFIELD;
#endif

typedef struct {
	unsigned short MemoMark;        /* N\0 */
	long     MemoLen;         	/* block num */
	long     MemoTime;
} dBITMEMO;

typedef struct tagDBTBLOCKINFO {
        long last;
        long next;
} DBTBLOCKINFO;

typedef struct tagDBTBLOCK {
        char data[DBT_DATASIZE];
        long last;
        long next;
} DBTBLOCK;

#define  dOK       0
#define  dNOT_OK   1

#define  IniBufUseTimes 7
#define  DBFVIEWFLAG            "#*#ViewDBF#*#"
#define  DBFVIEWFLAGLENGTH 13

#ifndef  DOPENPARA
    #include <fcntl.h>
    #include <share.h>
    #include <sys\stat.h>
    #define  DOPENPARA      (short)(O_RDWR|O_BINARY), (short)SH_DENYNO, (short)(S_IREAD|S_IWRITE)
#endif

/*typedef struct {
	unsigned buf_write:1;
	unsigned dbt_write:1;
	unsigned file_write:1;
	unsigned append:1;
	unsigned creat:1;
	int      SleepDBF;
} dFLAG;
change this define 1998.1.2 Xilong*/
typedef struct {
	char buf_write;
	char dbt_write;
	char file_write;
	char append;
	char creat;
	int  SleepDBF;
} dFLAG;

typedef struct {
	char     buf_write;
	short    times;
	long     rec_num;
} dBUFPOINTER;


typedef enum {
	dSEEK_SET,              /* from head    */
	dSEEK_CUR,              /* from current */
	dSEEK_END               /* from end     */
} dSEEK_POS;


typedef struct tagDfile {
	unsigned char dbf_flag; /* bit 7    6    5    4    3    2    1    0
				use as Memo                          DBF_FLAG
									    */
	unsigned char op_flag;
	char     name[NAMELEN]; /* DBF name */
	short    fp;            /* DBF handle */
	long     dp;            /* DBT handle */
	short    oflag;
	short    shflag;
	short    pmode;
	unsigned char last_modi_time[3]; /* DBF last modify time */
	unsigned short headlen;          /* DBF head length      */
	unsigned short field_num;        /* DBF fields number    */
	dFIELD   *field;                 /* fields described in structure */
	short 	 *fld_id;          	 /* index of field       */
	unsigned short rec_len;          /* record length        */
	unsigned char *rec_buf;          /* buffer of current record, remember
					    that dseek() can't fresh this
					    buffer                          */
	unsigned char *dbt_buf;          /* buffer if the dbf hasn't dbt it points to NULL */
	long int *dbt_head;              /* [0] DBT file's block number, if the
					    DBF file hasn's Memo field, this
					    value is no use, and user has no
					    right to use it                */
	long int dbt_len;                /* this record's DBT field block num */
	long int dbt_p;
	long int rec_num;                /* DBF record number    */
	long int rec_no;		 /* rec no */
	long int rec_p;                  /* pointer of current record,
					    sometime this doesn't mean the
					    record in rec_buf if rec_p     */
	long int rec_beg;                /* use for buf sink, don't use it!*/
	long int rec_end;                /* use for buf sink, don't use it!*/
	int 	 buf_bsize;        /* use for buf sink, don't use it!*/
	int 	 buf_rsize;        /* use for buf sink, don't use it!*/
	unsigned char *buf_sink;         /* use for buf sink, don't use it!*/
	dFLAG    write_flag;             /* write flag, see structure dFLAG*/
	unsigned short buf_usetimes;     /* use for buf sink, don't use it!*/
	unsigned short buf_freshtimes;   /* use for buf sink, don't use it!*/
	long int max_recnum;             /* use for buf sink, don't use it!*/
	long int min_recnum;             /* use for buf sink, don't use it!*/
	dBUFPOINTER *buf_pointer;        /* use for buf sink, don't use it!*/
	char     buf_methd;              /* 'S' or 'P'. means the current
					    buffer manage method           */
	short    error;                  /* some error occured during DBF
					    operating, it should be clear
					    by dclearerr(), and read by
					    dioerror()                     */
	char     		sql_id[IDLEN+1];
	char 			sqlCursor[CURSORNAMELEN+1];
	struct sqlda		*sqlDesc;
	char  	        	*lpSqlDataBuf;
	unsigned      		dIOReadWriteFlag;
	long                    firstDel;

	unsigned char           *rec_tmp;          // pointer of current record's end
	char                    syncBhNeed[DIO_MAX_SYNC_BTREE];
	void                    *bhs[DIO_MAX_SYNC_BTREE];       // it is bHEAD *,
								// but I cannot include btree.h
	short                   syncBhNum;
	unsigned short          del_link_id;
	long			*del_link_off;			//chars before
	char			keep_physical_order;
	char			dpack_auto;

	short			tbid;
	short			wlog;
	short			mainkey_num;
	short			mainkey_fields[dioMAX_KEYFIELD_NUM];

	short			isRecCheck;

	char			szAlias[10];

	char			cdbtIsGDC;

	unsigned short		fldTimeStampId;
	int			fldTimeStampOffset;
	int			fldTimeStampLen;

	int			slowPutRec;

	int 			lock_num;

#ifdef WSToMT
	CRITICAL_SECTION 	dCriticalSection;
	char			inCriticalSection;
	struct tagDfile		*pdf;   //pointer of public dFILE
	struct tagDfile		*pvdf;  //pointer of public dFILE
	unsigned long		timeStamp;

	short			*pwLocksByThread;	//locks pointer

#endif

} dFILE;

typedef struct{
    dFILE  source;
    char   name[NAMELEN]; 		 /* DBF name */
    dFILE  *view;
    char   *rec_buf;
    unsigned short RecnoFldid;
    short  field_num;
    char  **szfield;
    char  *ViewField;
    dFILE *awakedSource;
    dFILE *awakedView;
    char  *szASQL;
} dVIEW;

/******************* variable defination for dio self *********************/

typedef struct tagDioOpenFileMan {
	dFILE *df;
	unsigned char op_flag;
#ifndef WSToMT
	unsigned char sleepFlag[_DIOFILENUM_];
#endif
	char     FileManOpable;
} DIO_OPENFILE_MAN;


#ifdef RJ_SUPPORT
extern RJFILE   *rjFile;
#endif

#ifdef IS_DioMAIN

//WSToMT unsigned      dIOReadWriteFlag = 0;     /* 0 read, 1 write */
//WSToMT short         dERROR = 0;
//WSToMT short        dAbsDbf = 1;
WSToMT int         dERROR = 0;
WSToMT int         dAbsDbf = 1;

short         _DioOpenFileNum_ = 0;
short         _DioFileHandleNum_ = _DIOFILENUM_;
DIO_OPENFILE_MAN	 _DioOpenFile_[_DIOFILENUM_];
dFIELD viewField[2] = {{(unsigned char *)VIEW_RECNO_FIELD, 'O', 4, 0}, \
					   {(unsigned char *)"", 'O', 4, 0}};
unsigned char *szDbfViewFlag = DBFVIEWFLAG;
char  	     *dioTmpFile = "ILOVEDIO.DBF";

#ifdef WSToMT
CRITICAL_SECTION  dofnCE;
#endif
#else

//extern WSToMT short dERROR;
//extern WSToMT short dAbsDbf;
extern WSToMT int dERROR;
extern WSToMT int dAbsDbf;

extern short         _DioOpenFileNum_;
extern DIO_OPENFILE_MAN	 _DioOpenFile_[_DIOFILENUM_];
extern dFIELD	     viewField[2];
extern unsigned char *szDbfViewFlag;

extern short _DioFileHandleNum_;
extern char  *dioTmpFile;

#ifdef WSToMT
extern CRITICAL_SECTION  dofnCE;
#endif

#endif


/*
----------------------------------------------------------------------------
		     function prototypes
----------------------------------------------------------------------------*/

void    dsetbuf( int dBufSize );
	/* set the record I/O buffer's size, the max static valuse is 65000
	 */
short     dresetbuf( dFILE *df, int dBufSize );
	/* reset the DBF's buffer size
	 */
dFILE   *dopen( char *filename, short oflag, short shflag, short pmode );
	/* open a DBF and a DBT( if M field exist ) , initialize
	 * the I/O buffer. Now the pmode use by DIO
	 * oflag: O_RDONLY, O_RDWR, O_WRONLY with '|'
	 * shflag: SH_COMPAT, SH_DENYRW, SH_DENYWR, SH_DENYRD, SH_DENYNO
	 */
dFILE   *dAwake( char *FileName, short oflag, short shflag, short pmode );
	/*
	 */
short   dSetAwake(dFILE *df, dFILE **tdf);
dFILE   *dcreate( char *, dFIELD * );
	/* dopen with create
	 */
short   dflush( dFILE * );
	/* flush the I/O buffer to disk
	 */
short   drflush( dFILE * );
	/* flush the I/O buffer from disk
	 */
void    dclearerr( dFILE * );
	/* set the error value with 0 ( no error )
	 */
short   dioerror( dFILE * );
	/* get the error value
	 */
dFIELD *dfcopy( dFILE *, dFIELD * );
	/* copy a DBF field message to a structure dFIELD
	 */
short   deof( dFILE *df );
long    dtell( dFILE *df );
long    dseek( dFILE *df, long rec_offset, dSEEK_POS from_where );
unsigned char *getrec( dFILE *df );
#define GetRecord(a,b)          getrec(a); \
				memcpy((b), (a)->rec_buf, (a)->rec_len)
	/* get a record with users record buffer */
unsigned char *get1rec( dFILE *df );

unsigned char *putrec( dFILE *df );
#define PutRecord(a,b)          memcpy((a)->rec_buf, (b), (a)->rec_len); \
				putrec(a)
	/* put a record with users record buffer */
unsigned char *put1rec( dFILE *df );

short   PhyDbtDelete( dFILE *df, long block );
	/* Physical delete a dbt record
	 */
unsigned char *DbtRead( dFILE *df );
short   DbtWrite( dFILE *df );
long    dbt_seek( dFILE *df, long rec_offset, dSEEK_POS from_where );
long    dbtlen( dFILE *df );
	/* get the memo file length with block
	 */
long    DbtEof( dFILE *df );
	/* is we have been to the end of memo field.
	 * EOF return the length with block, else 0
	 */
short   dclose( dFILE *df );
	/* close Dfile
	 * return:
	 * 0: not fresh, 1:fresh, 2:error
	 */
#ifdef IS_DioMAIN
__declspec(dllexport) short dSleep( dFILE *df );
#else
__declspec(dllimport) short dSleep( dFILE *df );
#endif
	/*
	 */
short   dRelease( dFILE *df );
	/* if df is NULL, release all.
	 */
short   dpack( dFILE *df );
	/* pack a dFILE
	 */
short   memo_pack( dFILE *df );
	/* Memo file pack
	 */
short   drecopy( dFILE *sdf, long s_recnum, dFILE *tdf, long t_recnum );
	/* copy a record from one to another
	 */
short   dinsert( dFILE *df, long recnum, short insnum );
	/* insert some blank record
	 */
long daddrec(dFILE *df);
	/* add record in rec_buf
	 */
short   dzap( dFILE *df );
	/* sucess return 0, invalid dFILE return 1, else -1
	 */
short  *field_ident( dFILE *df, char **fld_name );
	/* identify the fld_name and set the index, return the number array 
	 * has been indetified
	 */
unsigned short  GetFldPosition( dFILE *df, char *fld_name );
	/* Get the position of field fld_name in df->rec_buf, the return 
	 * value is the first character's index position
	 */
unsigned short  GetFldid( dFILE *df, char *fld_name );
	/* Get the field id of field fld_name in df 
	 * success return the id, fail 0xFFFF
	 */
char  *get_fld( dFILE *df, unsigned short name, char *dest );
	/* get the field name(index number) to a string from rec_buf
	 */
short  put_fld( dFILE *df, unsigned short name, char *src );
	/* put the field name(index number) to rec_buf from src
	 */
void  *GetField( dFILE *df, unsigned short name, void *dest );
	/* get field with translate: number less or equal than 9999,
	 * return integer pointer, others long integer pointer; C,M,T
	 * type return the string pointer
	 */
short  PutField( dFILE *df, unsigned short name, void *src );
	/* put field with translate. see GetField()
	 */
#define append_blank(df)   append_n_blank( df, 1 )
	/* append a blank record
	 */
short   append_n_blank( dFILE *, short );
	/* append n blank record
	 */
short   PutDelChar( dFILE *df, unsigned char DelChar );
#define RecDelete(df)         PutDelChar( df, '*' )
#define RecRecall(df)         PutDelChar( df, ' ' )
unsigned char GetDelChar( dFILE *df );
#define YNRecDeleted(df)      GetDelChar( df ) - ' '
unsigned char *NewRec( dFILE *df );

unsigned char *NewBlobFlds( dFILE *df );

short   IsMemoExist( dFILE *df );
	/* Is the Dfile has Memo type field return not 1 else 0
	 */
//void    MemoBufInit( unsigned char *MemoBuf );
	/* Init the first block of memo buffer
	 */
short dEncrypt( char *DbfName );
	/*
	 */
short dDecrypt( char *DbfName );
	/*
	 */
short getFieldNum( dFILE *df );
dFIELD *getFieldInfo( dFILE *df, unsigned short name );
long getRecNum( dFILE *df );
long getRecP( dFILE *df );
long getReallyRecP( dFILE *df );
short makeView(char *viewName, char *dbfName, char *recFileName, ... );
long maintainRecP( dFILE *df, long recp );

//decrease the replay count, even close it
int  dDecreaseCount(dFILE *df);
void clrTempTableList( void );
void closeTempTableList( void );

char *dGetLine( int handle );
void MK_name( char * );
void ReadViewFileName(int handle, char *SourceFileName, char *ViewFileName);
void dSyncView( dVIEW *dv );

long allocDbtBlock(dFILE *df, long lastBlock);
int uncatchTable(char *FileName);

int absSyncDfBh(dFILE *df);


#ifdef WSToMT
void wmtDbfLock(dFILE *df);
void wmtDbfUnLock(dFILE *df);
int wmtDbfIsLock(dFILE *df);
int wmtDbfTryLock(dFILE *df);
#endif


/*<<<<<<<<<<<<<<< functions use for TV, don't use it by users >>>>>>>>>>>>>*/

unsigned short   BufFlush( dFILE * );
void    free_dFILE( dFILE * );

#endif
