/*************
 *
 * ndx manage utility
 *
 * Writen by Xilong Pei   Jan. 27  1999
 *
 **************************************************************************/

#ifndef __NDX_MAN_H__
#define __NDX_MAN_H__

int buildIndexAndRegThem(char *szDataBase, dFILE *df, char *ndxName, char *keys);
int dropIndexAndRegThem(char *szDataBase, dFILE *df, char *ndxName);


#endif
