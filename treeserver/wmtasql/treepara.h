/****************
* treepara.h
* Xilong Pei 1997.10
*****************************************************************************/

#ifndef __TREEPARA_H_
#define __TREEPARA_H_

#define  ASQL_MAXPARA    256


//this type is compitable with SysVarOFunType in XEXP.H
typedef struct tagTREESVR_PARA {
    short  type;               //STRING_IDEN
    char   var[32];
    char   *value;
    char   resSpace[28];        //[MAX_OPND_LENGTH-sizeof(char*)];
    short  len;
} TREESVR_PARA;

TREESVR_PARA *getTreeSvrPara(int len, const char *data);
char *setTreeSvrPara(TREESVR_PARA *tp, int *len);
int lookTSPara(TREESVR_PARA *tp, const char *var);
int countTSPara(TREESVR_PARA *tp);


#endif
