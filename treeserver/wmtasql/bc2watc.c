/****************
* bc2watc.c
* 1995.11.2
* copyright (c) EastUnion Computer Service Co., Ltd.
*****************************************************************************/

#include <stdio.h>
#include <dos.h>
#ifndef _MSC_VER
#ifdef _WATCOM_
#include <graph.h>

unsigned short getw(FILE *stream)
{
    union tagWC {
        unsigned short ui;
        unsigned char  uc[2];
    } wc;

    wc.uc[0] = getc(stream);
    wc.uc[1] = getc(stream);
    return  wc.ui;
}


unsigned short putw(unsigned short w, FILE *stream)
{
    union tagWC {
        unsigned short ui;
        unsigned char  uc[2];
    } wc;

    wc.ui = w;
    putc(wc.uc[0], stream);
    putc(wc.uc[1], stream);
    return  w;
}
#endif
#endif

#ifdef __N_C_S
void clrscr(void)
{
    _clearscreen( _GCLEARSCREEN );
}


void gotoxy(int x, int y)
{
        union REGS regs;

        regs.h.dh = y-1;
        regs.h.dl = x-1;
        regs.h.bh = 0;          //page no
        regs.h.ah = 0x02;
        int386(0x10, &regs, &regs);
//    _settextposition(y, x);
}



void textattr(short newattr)
{
    _settextcolor( newattr );
}
#endif

/*
main()
{
        FILE *file;
        short w;
        file = fopen("c:\\tg\\sys\\tgctl.ndx", "r+b");
        w = getww(file);
        printf("%d", w);
}*/
