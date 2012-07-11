/*-------------------------------------------*
 *
 *   FileName: diodbt.c
 *   Function: read memo filed from DBF file to another
 *                      text file for edit and put it back;
 *
 *   Author:   Zhaolin Shan, Xilong Pei
 *   Date:           1995/5/4
 *                   2001/7/7, add ODBC_BLOB support, not finished
 *-------------------------------------------------------------------------*/

#include <io.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <memory.h>
#else
#include <mem.h>
#endif
#include <string.h>
#include <time.h>
#include <limits.h>

#include "dio.h"
#include "diodbt.h"
#include "mistring.h"
#include "dbtree.h"
#include "sql_dio.h"
#include "odbc_dio.h"

//2001.7.7
//#define ODBC_BLOB_SUPPORT


extern char *GetDbtId(dFILE *df, char *start);
extern char *PutDbtId(dFILE *df, char *start);


/*
----------------------------------------------------------------------------
			dbtToFile()
---------------------------------------------------------------------------*/
long  dbtToFile( dFILE *df, unsigned short fldId, char *filename )
{
      int       fpDbt;
      long      len = 0;
      long      fileLen = 0;
      int       writeLen;

      //2001.7.6 Xilong Pei
      if( df->dbf_flag == SQL_TABLE )
	  return  -2;

#ifndef ODBC_BLOB_SUPPORT
      if( df->dbf_flag == ODBC_TABLE )
	  return  -2;
#endif

      fpDbt = open(filename, O_RDWR|O_CREAT|O_TRUNC|O_BINARY|SH_DENYWR, S_IWRITE);
      if( fpDbt < 0 )
	return -1;

      //------------
#ifdef ODBC_BLOB_SUPPORT
      if( df->dbf_flag != ODBC_TABLE )
#endif
	    wmtDbfLock(df);
      //------------

#ifdef ODBC_BLOB_SUPPORT
      if( df->dbf_flag == ODBC_TABLE ) {
	    char *sp = dioBlobSQLGetData(df, fldId);
	    if( sp != NULL ) {
		  write(fpDbt, sp, fileLen);
		  free( sp );
	    }
	    goto  BLOB_END_1;
      }
#endif
      
      if( df->cdbtIsGDC )
      { //DBT in G_D_C
	  char buf[256];
	  char *sp;

	  get_fld(df, fldId, buf);
	  len = atol(buf);

	  if( len > 0 ) {
	      sp = readBtreeData((bHEAD *)(df->dp), (char *)&len, NULL, 0);
	      if( sp != NULL ) {
		  fileLen = getBtreeLastReadSize();
		  write(fpDbt, sp, fileLen);
		  freeBtreeMem(sp);
	      }
	  }

      } else {
	  // use the dio buffer for read
	  if( GetField(df, fldId, NULL) != NULL )
	  {
	      DBTBLOCK *dblock = (DBTBLOCK *)df->dbt_buf;
	      dBITMEMO *bMemo = (dBITMEMO *)(df->dbt_buf);

	      if( bMemo->MemoMark == dBITMEMOMemoMark ) {
		  fileLen = bMemo->MemoLen;
		  writeLen = DBT_DATASIZE - sizeof(dBITMEMO);
	      } else {
		  fileLen = -1;
		  writeLen = DBT_DATASIZE;
	      }

	      while( 1 ) {
			if( writeLen != DBT_DATASIZE )
			    write(fpDbt, &(df->dbt_buf[sizeof(dBITMEMO)]), writeLen);
			else
			    write(fpDbt, df->dbt_buf, DBT_DATASIZE);

			if( dblock->next <= 0 ) {
			    break;
			} else {
			    lseek(df->dp, dblock->next*DBTBLOCKSIZE, SEEK_SET);
			}
			DbtRead(df);
			writeLen = DBT_DATASIZE;
			len++;
	      }
	      write(fpDbt, df->dbt_buf, DBT_DATASIZE);
	  }
       }

#ifdef ODBC_BLOB_SUPPORT
BLOB_END_1:
#endif

       if( fileLen >= 0 )
	   chsize(fpDbt, fileLen);

       close( fpDbt );

       //------------
#ifdef ODBC_BLOB_SUPPORT
      if( df->dbf_flag != ODBC_TABLE )
#endif
	    wmtDbfUnLock(df);
      //------------

       return len;

} // end of dbtToFile()


/*
----------------------------------------------------------------------------
			dbtFromFile()
* Caution:
*     User should call putrec() by himself
* if fileName = "NUL" will erase the blob field content
---------------------------------------------------------------------------*/
long  dbtFromFile( dFILE *df, unsigned short fldId, char *fileName )
{

     int       fpDbt;
     long      filelen;
     long      blockCount;
     long      i;
     long      memo_start;
     int       iw;
     char      *start;
     dBITMEMO  bMemo;

     //2001.7.6 Xilong Pei
     if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE )
	  return  -2;

	  /*
     // Add by Jingyu Niu, 2000.09.04
     // If fileName = "NUL" or fileName = "" will erase the blob field content
     if( !fileName || !(*fileName) ) {
	 // clear BLOB field
	 if( df->cdbtIsGDC ) { //DBT in G_D_C
	     long  id;
	     
	     //2000.8.14 Xilong
	     wmtDbfLock(df);
	     start = df->field[fldId].fieldstart;

	     if( (id = atol(subcopy(start,0,10))) > 0 ) {
			 freeBtreeData((bHEAD *)(df->dp), (char *)&id);

			 *start = '\0';
			 if( PutDbtId(df, start) == NULL ) {
				 wmtDbfUnLock(df);
				return  0;
			}
	     }	    

	    //PutField() has already write them
	    put1rec(df);
	    wmtDbfUnLock(df);

	    return 0;
	 }

	 // else if not in GDC file
	 // Not implemented.
     }
	 */

     fpDbt = open(fileName, O_RDONLY|O_BINARY);
     if( fpDbt < 0 )
	 return -1;

     filelen = filelength(fpDbt);

     //2000.7.21
     //if( filelen <= 0 )
     if( filelen < 0 )
     {
	close( fpDbt );
	return  0;
     }

     //1999.9.10 Xilong
     if( deof(df) )
     {
	//clear the BLOB field id buf, if the record has more than 1 BLOB
	//field, this is safe, for we have put1rec() next, and we test
	//deof() first.
	NewBlobFlds(df);

	put1rec(df);
	//return  -2;
     }


      if( df->cdbtIsGDC )
      { //DBT in G_D_C
	  char *sp;
	  long  id, idKey;

	  //2000.8.14 Xilong
	  wmtDbfLock(df);

	  if( filelen <= 0 ) 
	  { //2000.7.21
	     start = df->field[fldId].fieldstart;
	     *start = '\0';
	     if( PutDbtId(df, start) == NULL ) {
		wmtDbfUnLock(df);
		close( fpDbt );
		return  -1;
	     }

	     wmtDbfUnLock(df);
	     close( fpDbt );

	     return  filelen;
	  } //end of if( filelen <= 0 )

	  sp = malloc(filelen+32);
	  if( sp == NULL ) {
	      close( fpDbt );
	      return  0;
	  }
	  iw = read(fpDbt, sp, filelen);

	  //2001.3.7 add a tail '\0' compelled
	  if( iw > 0 ) {
	      sp[iw] = '\0';
	      filelen++;
	  }

	  start = df->field[fldId].fieldstart;

	  if( (id = atol(subcopy(start,0,10))) <= 0 ) {
	      idKey = LONG_MAX;
	      readBtreeData((bHEAD *)(df->dp), (char *)&idKey, (char *)&id, sizeof(long));
	      id++;
	      writeBtreeData((bHEAD *)(df->dp), (char *)&idKey, (char *)&id, sizeof(long));
	  }

	  writeBtreeData((bHEAD *)(df->dp), (char *)&id, sp, iw);

	  free(sp);

	  sprintf(start, "%9ld", id);
	  if( PutDbtId(df, start) == NULL ) {
		wmtDbfUnLock(df);
		close( fpDbt );
		return  0;
	  }

	  //PutField() has already write them
	  //put1rec(df);
	  wmtDbfUnLock(df);

	  close( fpDbt );

	  return  filelen;

      } //ELSE ELSE ELSE ELSE ELSE

      blockCount = (filelen + sizeof(dBITMEMO) + DBT_DATASIZE - 1) / DBT_DATASIZE;

      //memset(df->dbt_buf, 0x1A, DBT_DATASIZE);
      bMemo.MemoMark = dBITMEMOMemoMark;
      bMemo.MemoLen = filelen;
      bMemo.MemoTime = time(NULL);

      lseek(fpDbt, 0, SEEK_SET);
      memcpy(df->dbt_buf, &bMemo, sizeof(dBITMEMO));
      iw = read(fpDbt, &(df->dbt_buf[sizeof(dBITMEMO)]), DBT_DATASIZE-sizeof(dBITMEMO));
      
      //2001.3.7 add a tail '\0' compelled
      if( iw >= 0 ) {
          df->dbt_buf[iw+sizeof(dBITMEMO)] = '\0';
      }

      wmtDbfLock(df);

      if( filelen <= 0 ) 
      { //2000.7.21
	start = df->field[fldId].fieldstart;
	*start = '\0';
	PutDbtId(df, start);
      } else {
	memo_start = allocDbtBlock(df, 0);
	DbtWrite( df );
	start = df->field[fldId].fieldstart;
	sprintf(start, "%9ld", memo_start);
	PutDbtId(df, start);
      }

      for( i = 1; i< blockCount; i++ )
      {
	     //memset(df->dbt_buf, 0x1A, DBT_DATASIZE);
	     iw = read(fpDbt, df->dbt_buf, DBT_DATASIZE);
      
	     //2001.3.7 add a tail '\0' compelled
	     if( iw >= 0 ) {
		df->dbt_buf[iw] = '\0';
	     }

	     memo_start = allocDbtBlock(df, memo_start);
	     DbtWrite( df );
       }
       //PutField() has already write them
       //put1rec(df);
       wmtDbfUnLock(df);

       close( fpDbt );

       return blockCount;

}


/*
----------------------------------------------------------------------------
			blobToMem()
pay attention to free the memory alloced by this function
---------------------------------------------------------------------------*/
void *blobToMem(dFILE *df, unsigned short fldId, long *memSize)
{
      long      len = 0;
      int       writeLen;
      char      *mem = NULL;

      //2001.7.6 Xilong Pei
      if( df->dbf_flag == SQL_TABLE )
	  return  NULL;

#ifndef ODBC_BLOB_SUPPORT
      if( df->dbf_flag == ODBC_TABLE )
	  return  NULL;
#endif

       //------------
#ifdef ODBC_BLOB_SUPPORT
      if( df->dbf_flag != ODBC_TABLE )
#endif
	    wmtDbfLock(df);
      //------------

#ifdef ODBC_BLOB_SUPPORT
      if( df->dbf_flag == ODBC_TABLE ) {
	    char *sp = dioBlobSQLGetData(df, fldId);
	    if( sp != NULL ) {
		  return  sp;
	    }
	    return  NULL;
      }
#endif

      if( df->cdbtIsGDC )
      { //DBT in G_D_C
	  char buf[256];

	  get_fld(df, fldId, buf);
	  len = atol(buf);

	  if( len > 0 ) {
	      mem = readBtreeData((bHEAD *)(df->dp), (char *)&len, NULL, 0);
	      if( mem != NULL ) {
		  *memSize = getBtreeLastReadSize();
	      }
	  }

	  wmtDbfUnLock(df);
	  return  mem;

      } else {
      // use the dio buffer for read
	  if( GetField(df, fldId, NULL) != NULL )
	  {
	      DBTBLOCK *dblock = (DBTBLOCK *)df->dbt_buf;
	      dBITMEMO *bMemo = (dBITMEMO *)(df->dbt_buf);

	      if( bMemo->MemoMark == dBITMEMOMemoMark ) {
		  *memSize = bMemo->MemoLen;
		  writeLen = min(DBT_DATASIZE-sizeof(dBITMEMO), bMemo->MemoLen);
	      } else
	      { //not BLOB field
		  wmtDbfUnLock(df);
		  *memSize = 1;
		  return  NULL;
	      }

	      if( (mem = malloc(bMemo->MemoLen+16) ) == NULL )
	      {
		  wmtDbfUnLock(df);
		  *memSize = 2;
		  return  NULL;
	      }

	      while( 1 ) {
		  if( writeLen != DBT_DATASIZE ) {
		      if( len > DBT_DATASIZE - sizeof(dBITMEMO) ) {
			memcpy(mem+len, df->dbt_buf, writeLen);
		      } else {
			memcpy(mem+len, &(df->dbt_buf[sizeof(dBITMEMO)]), writeLen);
		      }
		      len += writeLen;
		  } else {
		      memcpy(mem+len, df->dbt_buf, DBT_DATASIZE);
		      len += DBT_DATASIZE;
		  }

		  if( dblock->next <= 0 || len >= *memSize ) {
		      break;
		  } else {
		      lseek(df->dp, dblock->next*DBTBLOCKSIZE, SEEK_SET);
		  }

		  DbtRead(df);

		  if( len + DBT_DATASIZE <= *memSize )
			writeLen = DBT_DATASIZE;
		  else
			writeLen = *memSize - len;
	      }     
	      *memSize = len;
	  }
       }

       wmtDbfUnLock(df);

       return mem;

} // end of blobToMem()


/*
----------------------------------------------------------------------------
			blobFromMem()
* Caution:
*     User should call putrec() by himself
* if memSize <= 0 will cause the program to erase the blob field
---------------------------------------------------------------------------*/
long  blobFromMem( dFILE *df, unsigned short fldId, char *mem, long memSize )
{
     long      blockCount;
     long      i, len;
     long      memo_start;
     char      *start;
     dBITMEMO  bMemo;

     //2001.7.6 Xilong Pei
     if( df->dbf_flag == SQL_TABLE || df->dbf_flag == ODBC_TABLE )
	  return  -2;

     //1999.9.10 Xilong
     if( deof(df) )
     {
	//clear the BLOB field id buf, if the record has more than 1 BLOB
	//field, this is safe, for we have put1rec() next, and we test
	//deof() first.
	NewBlobFlds(df);

	put1rec(df);
	//return  -2;
     }

      if( df->cdbtIsGDC )
      { //DBT in G_D_C
	  long  id, idKey;

	  //2000.9.6 Xilong Pei
	  wmtDbfLock(df);

	  if( memSize <= 0 )
	  { //2000.7.21
	    start = df->field[fldId].fieldstart;
	    *start = '\0';
	    if( PutDbtId(df, start) == NULL ) {
		wmtDbfUnLock(df);
		return  -1;
	    }
	    wmtDbfUnLock(df);

	    return  0;

	  } //end of if( memSize <= 0 )

	  start = df->field[fldId].fieldstart;

	  if( (id = atol(subcopy(start,0,10))) <= 0 ) {
	      idKey = LONG_MAX;
	      readBtreeData((bHEAD *)(df->dp), (char *)&idKey, (char *)&id, sizeof(long));
	      id++;
	      writeBtreeData((bHEAD *)(df->dp), (char *)&idKey, (char *)&id, sizeof(long));
	  }

	  writeBtreeData((bHEAD *)(df->dp), (char *)&id, mem, memSize);

	  sprintf(start, "%9ld", id);
	  if( PutDbtId(df, start) == NULL ) {
		wmtDbfUnLock(df);
		return  0;
	  }

	  //PutField() has already write them
	  //put1rec(df);
	  wmtDbfUnLock(df);

	  return  memSize;

      } //ELSE ELSE ELSE ELSE ELSE


     blockCount = (memSize + sizeof(dBITMEMO) + DBT_DATASIZE - 1) / DBT_DATASIZE;

     //memset(df->dbt_buf, 0x1A, DBT_DATASIZE);
     bMemo.MemoMark = dBITMEMOMemoMark;
     bMemo.MemoLen = memSize;
     bMemo.MemoTime = time(NULL);

     wmtDbfLock(df);

     memcpy(df->dbt_buf, &bMemo, sizeof(dBITMEMO));

     //the info size is less than one node
     if( memSize < DBT_DATASIZE-sizeof(dBITMEMO) ) {
	memcpy(&(df->dbt_buf[sizeof(dBITMEMO)]), mem, memSize);
	len = memSize;
     } else {
	memcpy(&(df->dbt_buf[sizeof(dBITMEMO)]), mem, DBT_DATASIZE-sizeof(dBITMEMO));
	len = DBT_DATASIZE-sizeof(dBITMEMO);
     }

     if( memSize <= 0 ) 
     {  //2000.7.21
	start = df->field[fldId].fieldstart;
	*start = '\0';
	PutDbtId(df, start);
     } else {
	memo_start = allocDbtBlock(df, 0);
	DbtWrite( df );
	start = df->field[fldId].fieldstart;
	sprintf(start, "%9ld", memo_start);
	PutDbtId(df, start);
     }

     for( i = 1;   i < blockCount;   i++ )
     {
	if( i < blockCount - 1 ) {
	    memcpy(df->dbt_buf, mem+len, DBT_DATASIZE);
	    len += DBT_DATASIZE;
	} else {
	    memcpy(df->dbt_buf, mem+len, memSize-len);
	}

	memo_start = allocDbtBlock(df, memo_start);
	DbtWrite( df );
     }

     //PutField() has already write them
     //put1rec(df);
     wmtDbfUnLock(df);

     return blockCount;

} //end of blobFromMem()


/*
----------------------------------------------------------------------------
			freeBlobMem()
* Caution:
*     User should call putrec() by himself
---------------------------------------------------------------------------*/
void freeBlobMem( dFILE *df, char *mem )
{
    if( df->cdbtIsGDC ) {
	freeBtreeMem(mem);
    } else {
	free(mem);
    }

} //end of freeBlobMem()





/*
static char dbtName[] = "dbtfile.mem";

void main( void )
{
	dFILE   *df;
	unsigned short  fldId;

	df = dopen( "SAMDBT.DBF", DOPENPARA );
	dseek( df, 0L, dSEEK_SET );
	while( !deof( df ) )
	{
		getrec( df );
		if( ( fldId = GetFldid( df, "MEMO" ) ) == -1 )
		{
			printf(" fall " );
			break;
		}
		dbtToFile( df, fldId, dbtName );
	 }

	 dseek( df, 0L, dSEEK_SET );
	 getrec( df );
	 if( ( fldId = GetFldid( df, "MEMO" ) ) == -1 )
	 {
		printf(" fall " );
		return;
	 }
	 fileToDbt( df, fldId, dbtName );
	 dclose( df );
}

  */
