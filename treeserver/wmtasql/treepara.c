/****************
* treepara.c
* Xilong Pei 1997.10
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "treepara.h"

TREESVR_PARA *getTreeSvrPara(int len, const char *data)
{
    char *mem;
    int  i, j, varNum;
    TREESVR_PARA *tp;
    
    if( (len <= 0 ) || (data == NULL) )
	return  NULL;

    /*
    //translate the string
    data = malloc(len);
    if( data == NULL )
        return  NULL;
    s = data;
    for(i = 0;   i < len;   i++) {
          c = odata[i];

          if( c == '@' ) {
              c = odata[++i];
              c1 = odata[++i];
              *s++ = (c - 'A') | ((c1 - 'A') << 4 );
          } else {
             *s++ = c;
          }
    }
    len = s - data;
    */

    mem = (char *)malloc(len + ASQL_MAXPARA*sizeof(TREESVR_PARA));
    if( mem == NULL ) {
	return  NULL;
    }

    i = 0;
    tp = (TREESVR_PARA *)mem;
    mem += ASQL_MAXPARA*sizeof(TREESVR_PARA);
    varNum = 0;
    while( i < len )
    {
        tp[varNum].type = 1051;         //STRING_IDEN;
        for(j = 0;  (data[i] != '\0') && (i < len);  j++, i++)
        {
              if( j < 30 )
                  tp[varNum].var[j] = data[i];
        }
        tp[varNum].var[j++] = '\0';
        i++;
	if( i >= len )	    break;

        tp[varNum].len = *(unsigned short *)&data[i];
        i+= sizeof(unsigned short);
	if( i >= len )	    break;
        mem += sizeof(unsigned short);

        tp[varNum].value = mem;
        for(j = 0;  (j < tp[varNum].len) && (i < len);  j++, i++)
              tp[varNum].value[j] = data[i];
        mem += j ;

        if( varNum < ASQL_MAXPARA-1 )
            varNum++;
        else
            break;

    } //end of while

    tp[varNum].var[0] = '\0';
    
    return  tp;

} //end of getTreeSvrPara()


char *setTreeSvrPara(TREESVR_PARA *tp, int *len)
{
    int  i, memSize = 1, oldSize, il;
    char *mem = NULL;

    mem = malloc(16);
    if( mem == NULL )
	return  NULL;
    *mem = '8';			//give it a mark

    for(i = 0;  (tp[i].var[0] != '\0') && (i < ASQL_MAXPARA);  i++)
    {
	oldSize = memSize;
	il = strlen(tp[i].var) + 1;
        memSize += il + sizeof(unsigned short) + tp[i].len;

        mem = realloc(mem, memSize);
        if( mem == NULL )
	    return  NULL;

	memcpy(mem+oldSize, tp[i].var, il);
	*(unsigned short *)(mem+oldSize+il) = tp[i].len;
	memcpy(mem+oldSize+il+sizeof(unsigned short), tp[i].value, tp[i].len);
    }

    *len = memSize;
    return  mem;

} //end of setTreeSvrPara()

int lookTSPara(TREESVR_PARA *tp, const char *var)
{
    int  i;

    for(i = 0;   tp[i].var[0] != '\0' && i < ASQL_MAXPARA; i++) {
          if( stricmp(tp[i].var, var) == 0 ) {
              return  i;
          }
    }

    return  -1;

} //end of lookTSPara()


int countTSPara(TREESVR_PARA *tp)
{
    int  i;

    for(i = 0;   tp[i].var[0] != '\0' && i < ASQL_MAXPARA; i++);

    return  i;

} //end of countTSPara()


/*
void main(void)
{
    char *s = "ASQL\AA\A\x0XilongPeiTREESVR\0\x3\x0NEW";
    char *s1;
    int  i;
    TREESVR_PARA *tp;
    TREESVR_PARA tp1[10];

     strcpy(tp1[0].var, "Shanghai");
     tp1[0].len = sizeof("People");
     tp1[0].value = "People";

     strcpy(tp1[1].var, "Shanghai");
     tp1[1].len = sizeof("People's republic");
     tp1[1].value = "People's republic";

     tp1[2].var[0] = '\0';

    s1 = setTreeSvrPara(tp1, &i);
    printf("Result len: %d\n", i);
    printf("%s\n", s1);

    tp = getTreeSvrPara(i, s1);

    for(i = 0;   tp[i].var[0] != '\0' && i < ASQL_MAXPARA; i++)
          printf("Var:%s Len:%d Value:%s\n", tp[i].var, tp[i].len, tp[i].value);

    free( tp );

    free( s1 );

} */


/************************* end of treepara.c ********************************/
