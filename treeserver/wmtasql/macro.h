/****************
* macro.h
* copyright (c) EastUnion Computer Service Co., Ltd. 1995
* author: MX Group
*****************************************************************************/


#ifndef _MACRO_H_
#define _MACRO_H_

typedef char *( *LOAD_STR_FUN)(char *);

char *macro( char *deststring, short maxlen, char *macrostring, LOAD_STR_FUN fun );

#endif