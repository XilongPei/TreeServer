/****************
*				t_int64.h
****************************************************************************/


#ifndef __T_INT64_H__
#define __T_INT64_H__

#define _TASQL_RADIX	95
//from 0x21-0x7E

char * _cdecl t_u64toa(unsigned _int64 val, char *buf);
_int64 _cdecl t_atoi64(const char *nptr);
int _cdecl t_atoi(const char *nptr);

#endif