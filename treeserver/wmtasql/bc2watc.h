/****************
* bc2watc.h
* 1995.11.2
* copyright (c) EastUnion Computer Service Co., Ltd.
*****************************************************************************/

#ifndef __BC2WATC_H
#define __BC2WATC_H

#ifndef _MSC_VER
#ifdef _WATCOM_
unsigned short getw(FILE *stream);
unsigned short putw(unsigned short w, FILE *stream);
#endif
#endif

#ifdef __N_C_S
void clrscr(void);
void gotoxy(int x, int y);
void textattr(short newattr);
#endif

#endif
