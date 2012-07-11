/***************
 *  Software for MIS
 *  Writen by: Xilong Pei, 1991,1992,1993,1998
 ****************************************************************************/

#define _InMistoolsProgramBody_

// this will make the btree give its runing message
//#define RuningMessageOn

#define TgmisSpecialCField

#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <search.h>
#include <limits.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>

#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#include <sys\locking.h>
#endif

#include "dio.h"
#include "mistring.h"
#include "mistools.h"
#include "btree.h"
#include "xexp.h"

#ifdef __N_C_S
#include "busyinfo.h"
#endif

char FLAG_subDbfCarry = 0;

/*
-------------------------------------------------------------------------
!                       DateMinusToDate
------------------------------------------------------------------------*/
DATETYPE  DateMinusToDate( char *Date1, char *Date2, char *Result )
{
    DATETYPE d1, d2, d3;
    char Dtemp1[10], Dtemp2[10];
    short months, days, cy, year, Date1BiggerThanDate2;
    char Days[] = {31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if(  ( Date1 == NULL ) || ( Date1 == NULL ) ) {
		*(long *)&d3 = LONG_MAX;
		return( d3 );
    }

    strcpy(Dtemp1, "00010101");
    strcpy(Dtemp2, "00010101");
    if( strcmp( Date1, Date2 ) > 0 ) {
	Date1BiggerThanDate2 = 1;
	strncpy(Dtemp1, Date1, min(8,strlen(Date1)));
	strncpy(Dtemp2, Date2, min(8,strlen(Date2)));
    } else {
	Date1BiggerThanDate2 = -1;
	strncpy(Dtemp1, Date2, 8);
	strncpy(Dtemp2, Date1, 8);
    }

    year = d1.year = atoi( substr(Dtemp1, 0, 4) );
    d1.month = atoi( substr(Dtemp1, 4, 2) );
    d1.day = atoi( substr(Dtemp1, 6, 2) );
    d2.year = atoi( substr(Dtemp2, 0, 4) );
    d2.month = atoi( substr(Dtemp2, 4, 2) );
    d2.day = atoi( substr(Dtemp2, 6, 2) );

    months = (d1.year-d2.year) * 12 + (d1.month - d2.month);

    if( ( days = d1.day - d2.day ) < 0 ) {
	months-- ;
	if( d1.month - 1 != 2 ) cy = Days[ d1.month - 1 ];
	else {
	    if( year % 4 == 0 ) cy = 29;
	    else cy = 28;
	    if( ( year % 100 == 0 ) && ( year % 400 != 0 ) ) cy = 28;
	}
	days += cy;
    }

    d3.year = months / 12 * Date1BiggerThanDate2;
    d3.month = months % 12;
    d3.day = (char)days;
    if( Date1BiggerThanDate2 >= 0 )
	   sprintf( Result, "%04d%02d%02d", d3.year, d3.month, d3.day );
    else   sprintf( Result, "%05d%02d%02d", d3.year, d3.month, d3.day );

    return( d3 );

}


/*
-------------------------------------------------------------------------
!                       DateMinusToLong
------------------------------------------------------------------------*/
long  DateMinusToLong( char *Date1, char *Date2)
{
    DATETYPE d1, d2;
    char Dtemp1[10], Dtemp2[10];
    short Days[] = { 0,31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,365};
    long days, days1, lRemainder;

    if(  ( Date1 == NULL ) || ( Date2 == NULL ) )  return( LONG_MAX );

    strcpy(Dtemp1, "00010101");
    strcpy(Dtemp2, "00010101");
    strncpy(Dtemp1, Date1, min(8,strlen(Date1)));
    strncpy(Dtemp2, Date2, min(8,strlen(Date2)));

    d1.year = atoi( substr(Dtemp1, 0, 4) );
    d1.month = atoi( substr(Dtemp1, 4, 2) );
    d1.day = atoi( substr(Dtemp1, 6, 2) );

    d2.year = atoi( substr(Dtemp2, 0, 4) );
    d2.month = atoi( substr(Dtemp2, 4, 2) );
    d2.day = atoi( substr(Dtemp2, 6, 2) );
    
    lRemainder = d1.year - 1;
    days1 = lRemainder / 400 *146097;
    lRemainder %= 400;
    days1 += lRemainder / 100 * 36524;
    lRemainder %= 100;
    days1 += lRemainder / 4 * 1461;
    lRemainder %= 4;
    days1 += lRemainder * 365;
    if(( ( d1.year % 100 != 0 ) || ( d1.year % 400 == 0 ) ) \
	     && ( d1.year % 4 == 0 ) && ( d1.month > 2 ) )
	    days1++;
    days1 += Days[ d1.month - 1 ] + d1.day;

    lRemainder = d2.year - 1;
    days = lRemainder / 400 *146097;
    lRemainder %= 400;
    days += lRemainder / 100 * 36524;
    lRemainder %= 100;
    days += lRemainder / 4 * 1461;
    lRemainder %= 4;
    days += lRemainder * 365;
    if(( ( d2.year % 100 != 0 ) || ( d2.year % 400 == 0 ) ) \
	     && ( d2.year % 4 == 0 ) && ( d2.month > 2 ) )
	    days++;
    days += Days[ d2.month - 1 ] + d2.day;
    
    return( days1 -days );

}



/*
-------------------------------------------------------------------------
!                       DateAddToLong
------------------------------------------------------------------------*/
long  DateAddToLong( char *Date, long DayNum, char *Result )
{
    DATETYPE d1, d2;
    char Dtemp[10];
    short Days[] = { 0,31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,365};
    long days, lRemainder;
    int nQuotient, i, inc;

    if( Date == NULL )    return( LONG_MAX );

    strcpy(Dtemp, "00010101");
    strncpy(Dtemp, Date, min(8,strlen(Date)));

    d1.year = atoi( substr(Dtemp, 0, 4) );
    d1.month = atoi( substr(Dtemp, 4, 2) );
    d1.day = atoi( substr(Dtemp, 6, 2) );
    
    lRemainder = d1.year - 1;
    days = lRemainder / 400 *146097;
    lRemainder %= 400;
    days += lRemainder / 100 * 36524;
    lRemainder %= 100;
    days += lRemainder / 4 * 1461;
    lRemainder %= 4;
    days += lRemainder * 365;
    if(( ( d1.year % 100 != 0 ) || ( d1.year % 400 == 0 ) ) \
	     && ( d1.year % 4 == 0 ) && ( d1.month > 2 ) )
	    days++;
    days += Days[ d1.month - 1 ] + d1.day;
    days += DayNum;

    nQuotient = days / 146097 * 400;
    days %= 146097;
    nQuotient += days / 36524 * 100;
    days %= 36524;
    nQuotient += days / 1461 * 4;
    days %= 1461;
    nQuotient += days / 365;
    days %= 365;
    d2.year = nQuotient + 1;

    if( ( ( d2.year % 100 != 0 ) || ( d2.year % 400 == 0 ) ) && \
		(d2.year % 4 == 0) )
	 inc = 1;
    else inc = 0;

    for( i = 1;  ; i++ ) {
	 if( i > 1 )   Days[i] += inc;
	 if( days <= Days[i] ) {
	     days -= Days[ i - 1 ];
	     d2.month = i;
	     d2.day = (char)days;
	     break;
	 }
    }
    sprintf( Result, "%04d%02d%02d", d2.year, d2.month, d2.day );

    return( *(long *)&d2 );

}



/*
**-------------------------------------------------------------------------
** dTableSum()
** add table stored in dfs into table stored in dft. if dft is NULL generate
** one, now the dft is a string pointer to mean the file name.
**------------------------------------------------------------------------*/
dFILE *  dTableSum(dFILE *dft, dFILE *dfs)
{
    unsigned short Fieldid[256];
    unsigned short i, InBuf;
    char 	   buf[256];
    double 	   lTemps, lTempt;
    //dFIELD 	  *Field;

    if( dft == NULL || dfs == NULL )
	return  NULL;

    for(i = 0;  i < dft->field_num;  i++)
	Fieldid[i] = GetFldid(dfs, dft->field[i].field);

    dseek(dft, 0L, dSEEK_SET);
    dseek(dfs, 0L, dSEEK_SET);

    while( !deof( dfs ) ) {

	getrec( dfs );

	// get the target field
	if( deof( dft ) ) {
		NewRec( dft );
	} else {
		get1rec( dft );
	}

	for(i = 0;  i < dft->field_num;  i++) {
		if( Fieldid[i] != 0xFFFF ) {

			InBuf = 0;

			memset(&lTempt, '\0', sizeof(double));
			// fitst give it a 0 for correct store short in long
			GetField(dft, i, &lTempt);

			if( dfs->field[ Fieldid[i] ].fieldtype == 'N' ) {
			     memset(&lTemps, 0, sizeof(double));
			     GetField(dfs, Fieldid[i], &lTemps);
			} else {
#ifdef TgmisSpecialCField
			     if( dfs->field[ Fieldid[i] ].fieldtype == 'C' ) {
					get_fld(dfs, Fieldid[i], buf);
					*(long *)&lTemps = atol( buf );
			     } else {
#endif
			     InBuf = 1;
			     get_fld(dfs, Fieldid[i], buf);
#ifdef TgmisSpecialCField
			     }
#endif
			} // end of else
			if( InBuf >= 1 ) {
			    put_fld( dft, i, buf );
			} else {
			    if( dfs->field[Fieldid[i]].fielddec <= 0 ) {
				if( dft->field[i].fielddec <= 0 )
					*(long *)&lTempt += *(long *)&lTemps;
				else
					lTempt += *(long *)&lTemps;
			    } else {
				if( dft->field[i].fielddec <= 0 )
					*(long *)&lTempt += (long)lTemps;
				else
					lTempt += lTemps;
			    }
			    PutField(dft, i, &lTempt);
			} // end of else

		} // end of if
	} // end of for

	putrec( dft );

    } // end of while

    return dft;

} // end of *dTableSum()

#undef _InMistoolsProgramBody_

/*
-------------------------------------------------------------------------
!                       DbfUniteOneToMore
! In this function we open a dbf with dopen() for the operate depends on the
! fields order, it means that the number in fld_id must be in order.
------------------------------------------------------------------------*/
short  DbfUniteOneToMore( short DbfNum, char *TargetDbf, \
						char *szField, char *dbf[] )
{
    dFILE **sdf, *tdf, *df;
    bHEAD **bh;
    char  *FieldName[2];
    dFIELD *dField;
    short i, j, k, FieldNum, FieldNum0;
    unsigned short pos[MAX_OPDBF];
    char buf[256];
    long int *RecNo;
    short GoOn;
    void *vp;

    if( DbfNum < 2 || DbfNum >= MAX_OPDBF ) {          return  4;      }

    if( (sdf = (dFILE **)calloc( DbfNum, sizeof( dFILE * ) )) == NULL ) {
	return  5;
    }
    if( (bh = (bHEAD **)calloc( DbfNum, sizeof( bHEAD * ) )) == NULL ) {
	free( sdf );
	return  5;
    }
    if( (RecNo = (long *)calloc( DbfNum, sizeof( long int ) )) == NULL ) {
	free( sdf );
	free( bh );
	return  5;
    }

    FieldName[0] = szField;
    FieldName[1] = NULL;

    // open df
    FieldNum = 0;
    for(i = 0;   i < DbfNum;  i++ ) {
	if( i == 0 ) {
		dsetbuf( 32000 );
	} else {
		dsetbuf( 0 );
	}

	sdf[i] = dopen(dbf[i], DOPENPARA);
	if( (pos[i] = GetFldid(sdf[i], FieldName[0])) == 0xFFFF ) {
		for( ;   i >= 0;   i-- ) {
			dclose( sdf[i] );
		}
		free( sdf );
		free( bh );
		free( RecNo );
		return  3;
	}
	FieldNum += sdf[i]->field_num - 1;
    }
    FieldNum += 2;

    if( (dField = (dFIELD *)calloc(FieldNum, sizeof(dFIELD)) ) == NULL ) {
	for( i = 0;   i < DbfNum;   i++ ) {
			dclose( sdf[i] );
	}
	free( sdf );
	free( bh );
	free( RecNo );
	return 1;
    }

    // get the new field message
    dfcopy(sdf[0], dField);
//    *(char *)( dField[ --FieldNum ].field ) = '\0';
    for(FieldNum = 0;  dField[FieldNum].field[0] != '\0';  FieldNum++);
    FieldNum0 = FieldNum;
    for(i = 1;  i < DbfNum;  i++) {
	for( j = 0;  j < sdf[i]->field_num;  j++ ) {
	    if( j != pos[i] ) {
		// check whether the field appeared
		vp = &(sdf[i]->field[j]);
		for( k = 0;   k < FieldNum;   k++ ) {
			if( stricmp( (char *)vp, dField[k].field ) == 0 ) {
				break;
			}
		}
		if( k >= FieldNum ) {
			memcpy( &(dField[FieldNum++]), vp, sizeof(dFIELD) );
		} else {
			//this field have already appeared and is not
			//the key field
			sdf[i]->fld_id[j] = -1;
		}
	    }
	}
    } // end of for

    // create the target
    if( (tdf = dcreate(TargetDbf, dField)) == NULL ) {
	for( i = 0;   i < DbfNum;   i++ ) {
		dclose( sdf[i] );
	}
	free( sdf );
	free( bh );
	free( RecNo );
	free( dField );
	return 2;
    }
    free( dField );
    dsetbuf( 0 );

    // prepare to generate the dbf
    for( i = 1;  i < DbfNum;  i++ ) {
	bh[i] = IndexOpen( (char *)sdf[i], dbf[i],  BTREE_FOR_OPENDBF );
	if( bh[i] == NULL )
	{
	    bh[i] = IndexBuild( (char *)sdf[i], FieldName, dbf[i],  BTREE_FOR_OPENDBF );
	}
    }

    dseek( sdf[0], 0L, dSEEK_SET );
    while( !deof( sdf[0] ) ) {
	getrec( sdf[0] );
	get_fld( sdf[0], pos[0], buf );
	for( i = 1;   i < DbfNum;    i++ ) {
		RecNo[i] = IndexSeek( bh[i],  buf );
	}
	do {
		for( i = 1;   i < DbfNum;   i++ ) {
		    if( RecNo[i] > 0 ) {
			dseek( sdf[i], RecNo[i]-1L, dSEEK_SET );
			getrec( sdf[i] );
		    } else {
			NewRec( sdf[i] );       // no record relation to df1
		    }
		}
		memcpy( tdf->rec_buf, sdf[0]->rec_buf, sdf[0]->rec_len );

		FieldNum = FieldNum0;
		for( i = 1;  i < DbfNum;  i++ ) {
		    df = sdf[i];
		    k = df->field_num;
		    for( j = 0;  j < k;   j++ ) {
			if( j != pos[i] && df->fld_id[j] >= 0 ) {
				put_fld(tdf, FieldNum++, get_fld(df, j, buf));
			}
		    }
		} // end of for

		putrec( tdf );

		GoOn = 0;
		for( i = 1;   i < DbfNum && GoOn == 0;   i++ ) {
			if( (RecNo[i] = IndexEqSkip( bh[i], 1L )) > 0L ) {
				GoOn = 1;
			}
		} // end of for

	} while ( GoOn != 0 );

    } // end of while

    // close files
    for( i = 0;  i < DbfNum;  i++ ) {
	dclose( sdf[i] );
    }
    dclose( tdf );

    // free memory
    free( sdf );
    free( bh );
    free( RecNo );

    return 0;

} // end of function DbfUniteOneToMore()

/*
-------------------------------------------------------------------------
!                       DbfInterSection
------------------------------------------------------------------------*/
short  DbfInterSection( short DbfNum, char *TargetDbf, \
				char *szField, char *dbf[] )
{
    dFILE **sdf, *tdf, *df;
    bHEAD **bh;
    char  *FieldName[2];
    dFIELD *dField;
    short i, j, k, FieldNum, FieldNum0;
    unsigned short pos[MAX_OPDBF];
    char buf[256];
    long int *RecNo;
    short GoOn;
    void *vp;

    if( DbfNum < 2 || DbfNum >= MAX_OPDBF ) {          return  4;      }

    if( (sdf = (dFILE **)calloc( DbfNum, sizeof( dFILE * ) )) == NULL ) {
	return  5;
    }
    if( (bh = (bHEAD **)calloc( DbfNum, sizeof( bHEAD * ) )) == NULL ) {
	free( sdf );
	return  5;
    }
    if( (RecNo = (long *)calloc( DbfNum, sizeof( long int ) )) == NULL ) {
	free( sdf );
	free( bh );
	return  5;
    }

    FieldName[0] = szField;
    FieldName[1] = NULL;

    // open df
    FieldNum = 0;
    dsetbuf( 32000 );
    for(i = 0;   i < DbfNum;  i++ ) {
	sdf[i] = dopen(dbf[i], DOPENPARA);
	if( (pos[i] = GetFldid(sdf[i], FieldName[0])) == 0xFFFF ) {
		for( ;   i >= 0;   i-- ) {
			dclose( sdf[i] );
		}
		free( sdf );
		free( bh );
		free( RecNo );
		dsetbuf( 0 );
		return  3;
	}
	FieldNum += sdf[i]->field_num - 1;
    }
    FieldNum += 2;

    if( (dField = (dFIELD *)calloc(FieldNum, sizeof(dFIELD)) ) == NULL ) {
	for( i = 0;   i < DbfNum;   i++ ) {
			dclose( sdf[i] );
	}
	free( sdf );
	free( bh );
	free( RecNo );
	dsetbuf( 0 );
	return 1;
    }

    // get the new field message
    dfcopy(sdf[0], dField);
//    *(char *)( dField[ --FieldNum ].field ) = '\0';
    for(FieldNum = 0;  dField[FieldNum].field[0] != '\0';  FieldNum++);
    FieldNum0 = FieldNum;
    for(i = 1;  i < DbfNum;  i++) {
	for( j = 0;  j < sdf[i]->field_num;  j++ ) {
	    if( j != pos[i] ) {
		// check whether the field appeared
		vp = &(sdf[i]->field[j]);
		for( k = 0;   k < FieldNum;   k++ ) {
			if( stricmp( (char *)vp, dField[k].field ) == 0 ) {
				break;
			}
		}
		if( k >= FieldNum ) {
			memcpy( &(dField[FieldNum++]), vp, sizeof(dFIELD) );
		} else {
			//this field have already appeared and is not
			//the key field
			sdf[i]->fld_id[j] = -1;
		}
	    }
	}
    } // end of for

    // create the target
    if( (tdf = dcreate(TargetDbf, dField)) == NULL ) {
	for( i = 0;   i < DbfNum;   i++ ) {
		dclose( sdf[i] );
	}
	free( sdf );
	free( bh );
	free( RecNo );
	free( dField );
	dsetbuf( 0 );
	return 2;
    }
    free( dField );
    dsetbuf( 0 );

    // prepare to generate the dbf
    for( i = 1;  i < DbfNum;  i++ ) {
	bh[i] = IndexOpen( (char *)sdf[i], dbf[i],  BTREE_FOR_OPENDBF );
	if( bh[i] == NULL )
	{
	    bh[i] = IndexBuild( (char *)sdf[i], FieldName, dbf[i],  BTREE_FOR_OPENDBF );
	}
    }

    dseek( sdf[0], 0L, dSEEK_SET );
    while( !deof( sdf[0] ) ) {
	getrec( sdf[0] );
	get_fld( sdf[0], pos[0], buf );
	for( i = 1;   i < DbfNum;    i++ ) {
		if( (RecNo[i] = IndexSeek( bh[i],  buf )) < 0 ) {
			// if there is no record in the sub set
			break;
		}
	} // end of for

	if( i < DbfNum )        continue;

	do {
		for( i = 1;   i < DbfNum;   i++ ) {
		    if( RecNo[i] > 0 ) {
			dseek( sdf[i], RecNo[i]-1L, dSEEK_SET );
			getrec( sdf[i] );
		    } else {
			NewRec( sdf[i] );       // no record relation to df1
		    }
		}
		memcpy( tdf->rec_buf, sdf[0]->rec_buf, sdf[0]->rec_len );

		FieldNum = FieldNum0;
		for( i = 1;  i < DbfNum;  i++ ) {
		    df = sdf[i];
		    k = df->field_num;
		    for( j = 0;  j < k;   j++ ) {
			if( j != pos[i] && df->fld_id[j] >= 0 ) {
				put_fld(tdf, FieldNum++, get_fld(df, j, buf));
			}
		    }
		} // end of for

		putrec( tdf );

		GoOn = 0;
		for( i = 1;   i < DbfNum && GoOn == 0;   i++ ) {
			if( (RecNo[i] = IndexEqSkip( bh[i], 1L )) > 0L ) {
				GoOn = 1;
			}
		} // end of for

	} while ( GoOn != 0 );

    } // end of while

    // close files
    for( i = 0;  i < DbfNum;  i++ ) {
	dclose( sdf[i] );
    }
    dclose( tdf );

    // free memory
    free( sdf );
    free( bh );
    free( RecNo );

    return 0;

} // end of function DbfInterSection()

/*
-------------------------------------------------------------------------
			     DbfCarryDbfs
Function:
    a dbf carry others dbf info, if there is carry it out, else blank
------------------------------------------------------------------------*/
int  DbfCarryDbfFlag = 0;

short  DbfCarryDbfs( short DbfNum, char *TargetDbf, \
			   char *szField, char *delFld, char *dbf[] )
{
    dFILE       **sdf, *tdf, *df;
    bHEAD       **bh;
    char        *FieldName[2];
    dFIELD      *dField;
    short       i, j, k, FieldNum, FieldNum0;
    unsigned short pos[MAX_OPDBF];
    char        buf[256];
    long int    *RecNo;
    long        lr;
//    short       GoOn;
    void        *vp;
    unsigned short delFldId;

    if( DbfNum < 2 || DbfNum >= MAX_OPDBF ) {          return  4;      }

    if( (sdf = (dFILE **)calloc( DbfNum, sizeof( dFILE * ) )) == NULL ) {
	return  5;
    }
    if( (bh = (bHEAD **)calloc( DbfNum, sizeof( bHEAD * ) )) == NULL ) {
	free( sdf );
	return  5;
    }
    if( (RecNo = (long *)calloc( DbfNum, sizeof( long int ) )) == NULL ) {
	free( sdf );
	free( bh );
	return  5;
    }

    FieldName[0] = szField;
    FieldName[1] = NULL;

    // open df
    FieldNum = 0;
    for(i = 0;   i < DbfNum;  i++ ) {
	if( i == 0 ) {
		dsetbuf( 32000 );
	} else {
		dsetbuf( 0 );
	}

	sdf[i] = dopen(dbf[i], DOPENPARA);
	if( (pos[i] = GetFldid(sdf[i], FieldName[0])) == 0xFFFF ) {
		for( ;   i >= 0;   i-- ) {
			dclose( sdf[i] );
		}
		free( sdf );
		free( bh );
		free( RecNo );
		dsetbuf( 0 );
		return  3;
	}
	FieldNum += sdf[i]->field_num - 1;
    }
    FieldNum += 2;
    if( (delFldId = GetFldid(sdf[0], delFld)) == 0xFFFF ) {
	for(i = 0;   i < DbfNum;  i++ ) {
		dclose( sdf[i] );
	}
	free( sdf );
	free( bh );
	free( RecNo );
	dsetbuf( 0 );
	return  3;
    }

    if( (dField = (dFIELD *)calloc(FieldNum, sizeof(dFIELD)) ) == NULL ) {
	for( i = 0;   i < DbfNum;   i++ ) {
			dclose( sdf[i] );
	}
	free( sdf );
	free( bh );
	free( RecNo );
	dsetbuf( 0 );
	return 1;
    }

    // get the new field message
    dfcopy(sdf[0], dField);
//    *(char *)( dField[ --FieldNum ].field ) = '\0';
    for(FieldNum = 0;  dField[FieldNum].field[0] != '\0';  FieldNum++);
    FieldNum0 = FieldNum;
    for(i = 1;  i < DbfNum;  i++) {
	for( j = 0;  j < sdf[i]->field_num;  j++ ) {
	    if( j != pos[i] ) {
		// check whether the field appeared
		vp = &(sdf[i]->field[j]);
		for( k = 0;   k < FieldNum;   k++ ) {
			if( stricmp( (char *)vp, dField[k].field ) == 0 ) {
				break;
			}
		}
		if( k >= FieldNum ) {
			memcpy( &(dField[FieldNum++]), vp, sizeof(dFIELD) );
		} else {
			//this field have already appeared and is not
			//the key field
			sdf[i]->fld_id[j] = -1;
		}
	    }
	}
    } // end of for

    // create the target
    if( (tdf = dcreate(TargetDbf, dField)) == NULL ) {
	for( i = 0;   i < DbfNum;   i++ ) {
		dclose( sdf[i] );
	}
	free( sdf );
	free( bh );
	free( RecNo );
	free( dField );
	dsetbuf( 0 );
	return 2;
    }
    free( dField );
    dsetbuf( 0 );

    if( FLAG_subDbfCarry ) {
	// prepare to generate the dbf
	for( i = 1;  i < DbfNum;  i++ ) {
	    bh[i] = IndexOpen( (char *)sdf[i], dbf[i],  BTREE_FOR_OPENDBF );
	    if( bh[i] == NULL )
	    {
		bh[i] = IndexBuild( (char *)sdf[i], FieldName, dbf[i],  BTREE_FOR_OPENDBF );
	    }
	    if( bh[i] == NULL )
	    { //the index cannot be created
		for( ;   i >= 1;   i-- ) {
			IndexClose(bh[i] );
		}
		for( i = 0;   i < DbfNum;   i++ ) {
			dclose( sdf[i] );
		}
		free( sdf );
		free( bh );
		free( RecNo );
		free( dField );
		dsetbuf( 0 );
		return 3;
	    }
	}
    }

    lr = 0;
    dseek( sdf[0], 0L, dSEEK_SET );
    while( !deof( sdf[0] ) ) {
	getrec( sdf[0] );

#ifdef RuningMessageOn
		euBusyInfo( ++lr );
#endif

	if( FLAG_subDbfCarry == 0 ) {
		NewRec(tdf);
		memcpy(tdf->rec_buf, sdf[0]->rec_buf, sdf[0]->rec_len);
		goto CarrySubSetEnd;
	}

	get_fld(sdf[0], delFldId, buf);
	if( atoi(buf) > 0 )
	{ //the record has been "deleted"
		continue;
	}

	memcpy(tdf->rec_buf, sdf[0]->rec_buf, sdf[0]->rec_len);
	get_fld( sdf[0], pos[0], buf );
	for( i = 1;   i < DbfNum;    i++ ) {
		//IndexKeyCount() will count the equal key and point to the
		//last one
		if( IndexKeyCount( bh[i], buf ) > 0 ) {
/*
			long l;

			l = IndexCurRecno(bh[i]);

			if ((unsigned)l > sdf[i]->rec_num)
			   l = sdf[i]->rec_num;

			dseek( sdf[i], l-1, dSEEK_SET );
*/
			getrec( sdf[i] );
		} else {
			NewRec( sdf[i] );       // no record relation to df1
		}
	}

	FieldNum = FieldNum0;
	for( i = 1;  i < DbfNum;  i++ ) {
		    df = sdf[i];
		    k = df->field_num;
		    for( j = 0;  j < k;   j++ ) {
			if( j != pos[i] && df->fld_id[j] >= 0 )
			{ //not key field and should be copied field
				put_fld(tdf, FieldNum++, get_fld(df, j, buf));
			}
/*                      if( DbfCarryDbfFlag == 31 ) {
			   if( (j != pos[i]) && (j != 1) ) {
				put_fld(tdf, FieldNum++, get_fld(sdf[i], j, buf));
			    }
			 }
			 else {
			   if( j != pos[i] ) {
				put_fld(tdf, FieldNum++, get_fld(sdf[i], j, buf));
			    }
			 }
*/
		    }
	} // end of for

CarrySubSetEnd:
	putrec( tdf );

    } // end of while

    // close files
    for( i = 0;  i < DbfNum;  i++ ) {
	dclose( sdf[i] );
    }

    if( FLAG_subDbfCarry ) {
	for( i = 1;  i < DbfNum;  i++ ) {
		IndexClose(bh[i]) ;
	}
    }

    dclose( tdf );

    // free memory
    free( sdf );
    free( bh );
    free( RecNo );

    return 0;

} // end of function DbfCarryDbfs()


long DbfLocateKey(dFILE *df, short field, char *key)
{
    char buf[256];

    while( !deof( df ) ) {
	getrec( df );
	get_fld(df, field, buf);
	if( strcmp(buf, key) == 0 ) {
		return  dseek(df, -1L, SEEK_CUR);
	}
    }

    return  -1;

} // end of function DbfLocateKey()



long DbfLocateExpr(dFILE *df, MidCodeType *m_c_code)
{
    while( !deof( df ) ) {
	getrec( df );
	if( CalExpr( m_c_code ) != 0 ) {
		return  dseek(df, -1L, SEEK_CUR);
	}
    }

    return  -1;

} // end of function DbfLocateExpr()


long DbfLocateSexpr(dFILE *df, char *szExpr)
{
    MidCodeType *m_c_code;

    m_c_code = WordAnalyse(szExpr);
    if( m_c_code == NULL )      return  -2;

    if( SymbolRegister(m_c_code,df,NULL,0,NULL,0) != NULL ) {
	FreeCode(m_c_code);
	return  -3;
    }
    while( !deof( df ) ) {
	getrec( df );
	if( CalExpr( m_c_code ) != 0 ) {
		FreeCode(m_c_code);
		return  dseek(df, -1L, SEEK_CUR);
	}
    }

    FreeCode(m_c_code);
    return  -1;

} // end of function DbfLocateSexpr()

/********************<<< end of software mistools.c >>>**********************/
