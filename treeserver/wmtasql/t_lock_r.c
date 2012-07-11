/*********
 * t_lock_r.c
 * 1999.1.17
 * ^-_-^
 ****************************************************************************/

 /*algorithm:

   1.a page like lock algorithm
   2.i means one lock
   3.one lock should be use only once, if it has been locked more than once
     once freeRecLock() call will free the lock

				 /  record M+1
				|   record M+2
   df->pwLocksByThread[i] ------+   record M+3
				|   record M+......
				 \  record M+N

   N = df->rec_num / TOTAL_LOCKS_PER_DF

 */


#define TOTAL_LOCKS_PER_DF	1024
#define R_LOCK_TRY_TIMES	3

#include <windows.h>

#include "wst2mt.h"
#include "dio.h"
#include "t_lock_r.h"


// 0 : success
// others : fail to lock it
/////////////////////////////////////////////////////////////////////////////
int toLockRec(dFILE *df, long recno)
{
    short    w;
    int	     i, j;

#ifdef WSToMT
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
    EnterCriticalSection( &(df->dCriticalSection) );
#endif

    if( df->pwLocksByThread == NULL ) {
	df->pwLocksByThread = malloc( df->lock_num * sizeof(short) );
	if( df->pwLocksByThread == NULL ) {
#ifdef WSToMT
		LeaveCriticalSection( &(df->dCriticalSection) );
#endif
		return  -1;
	}
	memset(df->pwLocksByThread, 0, df->lock_num * sizeof(short) );
    }

    i = recno / (df->rec_num / df->lock_num + 1);
    if( i >= df->lock_num )
	i = df->lock_num - 1;

    for(j = 0;  j < R_LOCK_TRY_TIMES;  j++) {
	w = df->pwLocksByThread[i];
	if( w == 0 || w == intOfThread )
		break;
	else
		Sleep(200);
    }

    if( w != 0 ) {
#ifdef WSToMT
	LeaveCriticalSection( &(df->dCriticalSection) );
#endif

	if( w == intOfThread )
		return  0;

	return  w;
    }

    df->pwLocksByThread[i] = intOfThread;

#ifdef WSToMT
    LeaveCriticalSection( &(df->dCriticalSection) );
#endif

    return  0;

} //end of toLockRec()


//
// 0 : success
/////////////////////////////////////////////////////////////////////////////
int freeRecLock(dFILE *df)
{
    int    i;

#ifdef WSToMT
    if( df->pdf != NULL ) {
	df = df->pdf;

	//if( (pdf->op_flag == VIEWFLAG) && ((dVIEW *)pdf)->view != NULL && pdf->pdf != NULL)
	if( df->pdf != NULL )
	{   //get the df handle in view original
	    //
	    //operating_view_handle.pdf ---> original_view_handle
	    //original_view_handle.pdf  ---> original_dFile_handle
	    //
	    df = df->pdf;
	}

    }
    EnterCriticalSection( &(df->dCriticalSection) );
#endif

    if( df->pwLocksByThread == NULL ) {
#ifdef WSToMT
	LeaveCriticalSection( &(df->dCriticalSection) );
#endif

	return  0;
    }

    for( i = 0;  i < df->lock_num;  i++ )
    {
	if( intOfThread == df->pwLocksByThread[i] )
	    df->pwLocksByThread[i] = 0;
    }

#ifdef WSToMT
    LeaveCriticalSection( &(df->dCriticalSection) );
#endif

    return  0;

} //end of freeRecLock()




///////////////////////// end of this file //////////////////////////////////