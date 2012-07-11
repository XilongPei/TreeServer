/////////////////
// ASQL_Optimize_config program
//
// copyright (c) 2000 Xilong Pei
//
/////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

#include "dio.h"
#include "cfg_fast.h"
#include "ts_dict.h"

//define in Btree.c
//cache size, in block
/////////////////////////////////////////////////////////////////////////////
extern short MAX_BtreeNodeBufferSize;
extern short DEF_BtreeNodeBufferSize;

//define in BtreeAdd.c
//sort memory size, in byte
/////////////////////////////////////////////////////////////////////////////
#ifdef __BORLANDC__
extern unsigned short        maxBtreeSinkSize;
#else
extern int      	     maxBtreeSinkSize;
#endif

short setASQLShortOptPara( int i, short w )
{
    switch( i ) {
	case 1:
	    MAX_BtreeNodeBufferSize = abs(w);
	    break;
	case 2:
	    DEF_BtreeNodeBufferSize = abs(w);
	    break;
	default:
	    return  0;
    }
    return  w;

} //setASQLShortOptPara()



long setASQLLongOptPara( int i, long L)
{
    switch( i ) {
	case 1:
	    dsetbuf( abs(L) );
	    break;
	case 2:
	    maxBtreeSinkSize = abs(L);
	    break;
	default:
	    return  0;
    }
    return  L;

} //setASQLLongOptPara()


/////////////////////////////////////////////////////////////////////////////

struct ASQLRunEnv {
    int   id;
    char  type;		//'w', 'L'
    char *configStr;
} ASQLEnv[] = {
    {1, 'w', "MAX_BtreeNodeBufferSize"},	//�����ռ佨��ʱ�����С���Կ�Ϊ��λ
    {2, 'w', "DEF_BtreeNodeBufferSize"},	//�����ռ�����ʱ�����С���Կ�Ϊ��λ
    {1, 'L', "dsetbuf"},			//��ֲ����棬�ֽ�Ϊ��λ
    {2, 'L', "maxBtreeSinkSize"}		//��������ռ��С���ֽ�Ϊ��λ
};

#define ASQLEnvNum 	sizeof(ASQLEnv)/sizeof(struct ASQLRunEnv)

/////////////////////////////////////////////////////////////////////////////


int initASQLRunEnv( void )
{
    char *sz;
    int   i;

    for( i = 0;  i < ASQLEnvNum;  i++ ) {
	sz = GetCfgKey(csuDataDictionary, "SYSTEM", "CONFIG", ASQLEnv[i].configStr);
	if( sz != NULL ) {
	    if( ASQLEnv[i].type == 'w' )
		setASQLShortOptPara( ASQLEnv[i].id, (short)atoi(sz) );
	    else if( ASQLEnv[i].type == 'L' )
		setASQLLongOptPara( ASQLEnv[i].id, atoi(sz) );
	}
    }

    return  0;

} //initASQLRunEnv()
