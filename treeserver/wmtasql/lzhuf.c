/****************
 * LZHUF.C
 * 1999.5
 *
 * author: Wang Chao
 *         ^-_-^
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <io.h>
#include <windows.h>

#include "ts_com.h"
#include "ts_const.h"
#include "exchbuff.h"
#include "terror.h"
//#include "tsftp.h"


/* LZSS Parameters */

#define N		4096	/* Size of string buffer */
#define F		60	/* Size of look-ahead buffer */
#define THRESHOLD	2
#define NIL		N	/* End of tree's node  */

/* Huffman coding parameters */

#define N_CHAR  	(256 - THRESHOLD + F)
					/* character code (= 0..N_CHAR-1) */
#define T 		(N_CHAR * 2 - 1)	/* Size of table */
#define R 		(T - 1)			/* root position */
#define MAX_FREQ	0x8000
					/* update when cumulative frequency */
					/* reaches to this value */
#define EXIT_OK 0
#define EXIT_FAILED -1
typedef unsigned char uchar;

/*compress file or memory*/
typedef enum CompressMode{
	MemMode,
	FileMode
} CCompressMode;

//typedef enum Memorigin{
//	Memseek_BEGIN,
//	Memseek_END,
//	Memseek_CUR
//};

/*structure fo memory such as file*/
typedef struct tagMemInfo{
	char *pMem;
	long CurPos;
	long length;
	FILE  *inFile;		//input file, take place of in memory
	FILE  *outFile;		//output file, take place of out memory
	EXCHNG_BUF_INFO *lpExchBufInfo;
	int    fEof;
} CMemInfo;

//structure holding global variable
typedef struct tagLSZZInfo {
	CCompressMode Mode;
	CMemInfo inMemInfo, outMemInfo;
	FILE  *infile, *outfile;
	unsigned long int  textsize, codesize, printcount, decodesize;
	unsigned char	text_buf[N + F - 1];
	short		match_position, match_length, lson[N + 1], rson[N + 257], dad[N + 1];
	unsigned short freq[T + 1];	//cumulative freq table
	//
	// pointing parent nodes.
	// area [T..(T + N_CHAR - 1)] are pointers for leaves
	//
	short prnt[T + N_CHAR];
	//pointing children nodes (son[], son[] + 1)
	short son[T];
	unsigned short getbuf;
	uchar getlen;
	unsigned putbuf;
	uchar putlen;
	unsigned code, len;
} CLSZZInfo;

void InitTree(CLSZZInfo *LSZZInfo);		/* Initializing tree */
void InsertNode(short r, CLSZZInfo *LSZZInfo);  /* Inserting node to the tree */
void DeleteNode(short p, CLSZZInfo *LSZZInfo);  /* Deleting node from the tree */

//FILE  *infile, *outfile;

void initialVar(CLSZZInfo *LSZZInfo);
//such as getc() operating in memory mode
int Memgetc(CMemInfo *MemInfo);
//such as putc() operating in memory mode
int Memputc(int c, CMemInfo *MemInfo);
//such as seek() operating in memory mode
/*int Memseek(CMemInfo *MemInfo, long offset, int origin);
//such as tell() operating in memory mode
long Memtell(CMemInfo *MemInfo);
//such as rewind() operating in memory mode
void Memrewind(CMemInfo *MemInfo);
*/
//such as fread() operating in memory mode
size_t Memread(void *buffer, size_t size, size_t count, CMemInfo *MemInfo);
//such as fwrite() operating in memory mode
size_t Memwrite(const void *buffer, size_t size, size_t count, CMemInfo *MemInfo);
//get the length of memory
size_t MemLength(CMemInfo *MemInfo);

//unsigned long int  textsize = 0, codesize = 0, printcount = 0;
short GetBit(CLSZZInfo *LSZZInfo);	/* get one bit */
short GetByte(CLSZZInfo *LSZZInfo);	/* get a byte */
void Putcode(short l, unsigned short c, CLSZZInfo *LSZZInfo);		/* output c bits */
void StartHuff(CLSZZInfo *LSZZInfo);
void reconst(CLSZZInfo *LSZZInfo);
void update(short c, CLSZZInfo *LSZZInfo);
void EncodeChar(unsigned short c, CLSZZInfo *LSZZInfo);
void EncodePosition(unsigned short c, CLSZZInfo *LSZZInfo);
void EncodeEnd(CLSZZInfo *LSZZInfo);
short DecodeChar(CLSZZInfo *LSZZInfo);
short DecodePosition(CLSZZInfo *LSZZInfo);
int Encode(CLSZZInfo *LSZZInfo);  	/* Encoding/Compressing */
int Decode(CLSZZInfo *LSZZInfo);  	/* Decoding/Uncompressing */

//unsigned char	text_buf[N + F - 1];
//short		match_position = 0, match_length = 0, lson[N + 1], rson[N + 257], dad[N + 1];

void InitTree(CLSZZInfo *LSZZInfo)  /* Initializing tree */
{
	short  i;

	for (i = N + 1; i <= N + 256; i++)
		LSZZInfo->rson[i] = NIL;			/* root */
	for (i = 0; i < N; i++)
		LSZZInfo->dad[i] = NIL;			/* node */
}

void InsertNode(short r, CLSZZInfo *LSZZInfo)  /* Inserting node to the tree */
{
	short  i, p, cmp;
	unsigned char  *key;
	unsigned short c;

	cmp = 1;
	key = &(LSZZInfo->text_buf[r]);
	p = N + 1 + key[0];
	LSZZInfo->rson[r] = LSZZInfo->lson[r] = NIL;
	LSZZInfo->match_length = 0;
	for ( ; ; ) {
		if (cmp >= 0) {
			if (LSZZInfo->rson[p] != NIL)
				p = LSZZInfo->rson[p];
			else {
				LSZZInfo->rson[p] = r;
				LSZZInfo->dad[r] = p;
				return;
			}
		} else {
			if (LSZZInfo->lson[p] != NIL)
				p = LSZZInfo->lson[p];
			else {
				LSZZInfo->lson[p] = r;
				LSZZInfo->dad[r] = p;
				return;	
			}
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - LSZZInfo->text_buf[p + i]) != 0)
				break;
		if (i > THRESHOLD) {
			if (i > LSZZInfo->match_length) {
				LSZZInfo->match_position = ((r - p) & (N - 1)) - 1;
				if ((LSZZInfo->match_length = i) >= F)
					break;
			}
			if (i == LSZZInfo->match_length) {
				if ((c = ((r - p) & (N - 1)) - 1) < LSZZInfo->match_position) {
					LSZZInfo->match_position = c;
				}
			}
		}
	}
	LSZZInfo->dad[r] = LSZZInfo->dad[p];
	LSZZInfo->lson[r] = LSZZInfo->lson[p];
	LSZZInfo->rson[r] = LSZZInfo->rson[p];
	LSZZInfo->dad[LSZZInfo->lson[p]] = r;
	LSZZInfo->dad[LSZZInfo->rson[p]] = r;
	if (LSZZInfo->rson[LSZZInfo->dad[p]] == p)
		LSZZInfo->rson[LSZZInfo->dad[p]] = r;
	else
		LSZZInfo->lson[LSZZInfo->dad[p]] = r;
	LSZZInfo->dad[p] = NIL;  /* remove p */
}

void DeleteNode(short p, CLSZZInfo *LSZZInfo)  /* Deleting node from the tree */
{
	short  q;

	if (LSZZInfo->dad[p] == NIL)
		return;			/* unregistered */
	if (LSZZInfo->rson[p] == NIL)
		q = LSZZInfo->lson[p];
	else
	if (LSZZInfo->lson[p] == NIL)
		q = LSZZInfo->rson[p];
	else {
		q = LSZZInfo->lson[p];
		if (LSZZInfo->rson[q] != NIL) {
			do {
				q = LSZZInfo->rson[q];
			} while (LSZZInfo->rson[q] != NIL);
			LSZZInfo->rson[LSZZInfo->dad[q]] = LSZZInfo->lson[q];
			LSZZInfo->dad[LSZZInfo->lson[q]] = LSZZInfo->dad[q];
			LSZZInfo->lson[q] = LSZZInfo->lson[p];
			LSZZInfo->dad[LSZZInfo->lson[p]] = q;
		}
		LSZZInfo->rson[q] = LSZZInfo->rson[p];
		LSZZInfo->dad[LSZZInfo->rson[p]] = q;
	}
	LSZZInfo->dad[q] = LSZZInfo->dad[p];
	if (LSZZInfo->rson[LSZZInfo->dad[p]] == p)
		LSZZInfo->rson[LSZZInfo->dad[p]] = q;
	else
		LSZZInfo->lson[LSZZInfo->dad[p]] = q;
	LSZZInfo->dad[p] = NIL;
}

/*
 * Tables for encoding/decoding upper 6 bits of
 * sliding dictionary pointer
 */
/* encoder table */
uchar p_len[64] = {
	0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

uchar p_code[64] = {
	0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
	0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
	0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
	0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
	0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
	0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* decoder table */
uchar d_code[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
	0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
	0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
	0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
	0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
	0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
	0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
	0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
	0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
	0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
	0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
	0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B,
	0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};

uchar d_len[256] = {
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

//unsigned short freq[T + 1];	/* cumulative freq table */

/*
 * pointing parent nodes.
 * area [T..(T + N_CHAR - 1)] are pointers for leaves
 */
//short prnt[T + N_CHAR];

/* pointing children nodes (son[], son[] + 1)*/
//short son[T];

//unsigned short getbuf = 0;
//uchar getlen = 0;

short GetBit(/*void*/CLSZZInfo *LSZZInfo)	/* get one bit */
{
	short i;

	while (LSZZInfo->getlen <= 8) {
		if ( LSZZInfo->Mode == FileMode ){
			if ((i = getc(LSZZInfo->infile)) < 0) i = 0;
		} else {
			if ((i = Memgetc(&(LSZZInfo->inMemInfo))) < 0) i = 0;
		};
		LSZZInfo->getbuf |= i << (8 - LSZZInfo->getlen);
		LSZZInfo->getlen += 8;
	}
	i = LSZZInfo->getbuf;
	LSZZInfo->getbuf <<= 1;
	LSZZInfo->getlen--;
	return (i < 0);
}

short GetByte(CLSZZInfo *LSZZInfo)	/* get a byte */
{
	unsigned short i;

	while (LSZZInfo->getlen <= 8) {
		if ( LSZZInfo->Mode == FileMode ){
			if ((i = getc(LSZZInfo->infile)) < 0) i = 0;
		} else {
			if ((i = (short)Memgetc(&(LSZZInfo->inMemInfo))) < 0) i = 0;
		};
		LSZZInfo->getbuf |= i << (8 - LSZZInfo->getlen);
		LSZZInfo->getlen += 8;
	}
	i = LSZZInfo->getbuf;
	LSZZInfo->getbuf <<= 8;
	LSZZInfo->getlen -= 8;
	return i >> 8;
}

//unsigned putbuf = 0;
//uchar putlen = 0;

void Putcode(short l, unsigned short c, CLSZZInfo *LSZZInfo)		/* output c bits */
{
	LSZZInfo->putbuf |= c >> LSZZInfo->putlen;
	if ((LSZZInfo->putlen += l) >= 8) {
		if ( LSZZInfo->Mode == FileMode ){
			putc(LSZZInfo->putbuf >> 8, LSZZInfo->outfile);
			if ((LSZZInfo->putlen -= 8) >= 8) {
				putc(LSZZInfo->putbuf, LSZZInfo->outfile);
				LSZZInfo->codesize += 2;
				LSZZInfo->putlen -= 8;
				LSZZInfo->putbuf = c << (l - LSZZInfo->putlen);
			} else {
				LSZZInfo->putbuf <<= 8;
				LSZZInfo->codesize++;
			}
		} else {
			Memputc(LSZZInfo->putbuf >> 8, &(LSZZInfo->outMemInfo));
			if ((LSZZInfo->putlen -= 8) >= 8) {
				Memputc(LSZZInfo->putbuf, &(LSZZInfo->outMemInfo));
				LSZZInfo->codesize += 2;
				LSZZInfo->putlen -= 8;
				LSZZInfo->putbuf = c << (l - LSZZInfo->putlen);
			} else {
				LSZZInfo->putbuf <<= 8;
				LSZZInfo->codesize++;
			}
		};
	}
}


/* initialize freq tree */

void StartHuff(CLSZZInfo *LSZZInfo)
{
	short i, j;

	for (i = 0; i < N_CHAR; i++) {
		LSZZInfo->freq[i] = 1;
		LSZZInfo->son[i] = i + T;
		LSZZInfo->prnt[i + T] = i;
	}
	i = 0; j = N_CHAR;
	while (j <= R) {
		LSZZInfo->freq[j] = LSZZInfo->freq[i] + LSZZInfo->freq[i + 1];
		LSZZInfo->son[j] = i;
		LSZZInfo->prnt[i] = LSZZInfo->prnt[i + 1] = j;
		i += 2; j++;
	}
	LSZZInfo->freq[T] = 0xffff;
	LSZZInfo->prnt[R] = 0;
}


/* reconstruct freq tree */

void reconst(CLSZZInfo *LSZZInfo)
{
	short i, j, k;
	unsigned short f, l;

	/* halven cumulative freq for leaf nodes */
	j = 0;
	for (i = 0; i < T; i++) {
		if (LSZZInfo->son[i] >= T) {
			LSZZInfo->freq[j] = (LSZZInfo->freq[i] + 1) / 2;
			LSZZInfo->son[j] = LSZZInfo->son[i];
			j++;
		}
	}
	/* make a tree : first, connect children nodes */
	for (i = 0, j = N_CHAR; j < T; i += 2, j++) {
		k = i + 1;
		f = LSZZInfo->freq[j] = LSZZInfo->freq[i] + LSZZInfo->freq[k];
		for (k = j - 1; f < LSZZInfo->freq[k]; k--);
		k++;
		l = (j - k) * 2;
		
		/* movmem() is Turbo-C dependent
		   rewritten to memmove() by Kenji */
		
		/* movmem(&freq[k], &freq[k + 1], l); */
		(void)memmove(&(LSZZInfo->freq[k + 1]), &(LSZZInfo->freq[k]), l);
		LSZZInfo->freq[k] = f;
		/* movmem(&son[k], &son[k + 1], l); */
		(void)memmove(&(LSZZInfo->son[k + 1]), &(LSZZInfo->son[k]), l);
		LSZZInfo->son[k] = i;
	}
	/* connect parent nodes */
	for (i = 0; i < T; i++) {
		if ((k = LSZZInfo->son[i]) >= T) {
			LSZZInfo->prnt[k] = i;
		} else {
			LSZZInfo->prnt[k] = LSZZInfo->prnt[k + 1] = i;
		}
	}
}


/* update freq tree */

void update(short c, CLSZZInfo *LSZZInfo)
{
	short i, j, k, l;

	if (LSZZInfo->freq[R] == MAX_FREQ) {
		reconst(LSZZInfo);
	}
	c = LSZZInfo->prnt[c + T];
	do {
		k = ++(LSZZInfo->freq[c]);

		/* swap nodes to keep the tree freq-ordered */
		if (k > LSZZInfo->freq[l = c + 1]) {
			while (k > LSZZInfo->freq[++l]);
			l--;
			LSZZInfo->freq[c] = LSZZInfo->freq[l];
			LSZZInfo->freq[l] = k;

			i = LSZZInfo->son[c];
			LSZZInfo->prnt[i] = l;
			if (i < T) LSZZInfo->prnt[i + 1] = l;

			j = LSZZInfo->son[l];
			LSZZInfo->son[l] = i;

			LSZZInfo->prnt[j] = c;
			if (j < T) LSZZInfo->prnt[j + 1] = c;
			LSZZInfo->son[c] = j;

			c = l;
		}
	} while ((c = LSZZInfo->prnt[c]) != 0);	/* do it until reaching the root */
}

//unsigned code = 0, len = 0;

void EncodeChar(unsigned short c, CLSZZInfo *LSZZInfo)
{
	unsigned short i;
	short j, k;

	i = 0;
	j = 0;
	k = LSZZInfo->prnt[c + T];

	/* search connections from leaf node to the root */
	do {
		i >>= 1;

		/*
		if node's address is odd, output 1
		else output 0
		*/
		if (k & 1) i += 0x8000;

		j++;
	} while ((k = LSZZInfo->prnt[k]) != R);
	Putcode(j, i, LSZZInfo);
	LSZZInfo->code = i;
	LSZZInfo->len = j;
	update(c, LSZZInfo);
}

void EncodePosition(unsigned short c, CLSZZInfo *LSZZInfo)
{
	unsigned short i;

	/* output upper 6 bits with encoding */
	i = c >> 6;
	Putcode(p_len[i], (unsigned short)(p_code[i] << 8), LSZZInfo);

	/* output lower 6 bits directly *////(c==0010 1011 & 0x3f==0011 1111)  << 10 -> 1010 1100 0000 0000
	Putcode((short)6, (unsigned short)((c & 0x3f) << 10), LSZZInfo);
}

void EncodeEnd(CLSZZInfo *LSZZInfo)
{
	if ( LSZZInfo->Mode == FileMode ){
		if (LSZZInfo->putlen) {
			putc(LSZZInfo->putbuf >> 8, LSZZInfo->outfile);
			(LSZZInfo->codesize)++;
		}
	} else {
		if (LSZZInfo->putlen) {
			Memputc(LSZZInfo->putbuf >> 8, &(LSZZInfo->outMemInfo));
			(LSZZInfo->codesize)++;
		}
	}
}

short DecodeChar(CLSZZInfo *LSZZInfo)
{
	unsigned short c;

	c = LSZZInfo->son[R];

	/*
	 * start searching tree from the root to leaves.
	 * choose node #(son[]) if input bit == 0
	 * else choose #(son[]+1) (input bit == 1)
	 */
	while (c < T) {
		c += GetBit(LSZZInfo);
		c = LSZZInfo->son[c];
	}
	c -= T;
	update(c, LSZZInfo);
	return c;
}

short DecodePosition(CLSZZInfo *LSZZInfo)
{
	unsigned short i, j, c;

	/* decode upper 6 bits from given table */
	i = GetByte(LSZZInfo);
	c = (unsigned short)d_code[i] << 6;
	j = d_len[i];

	/* input lower 6 bits directly */
	j -= 2;
	while (j--) {
		i = (i << 1) + GetBit(LSZZInfo);
	}
	return c | i & 0x3f;
}

/* Compression */


/* Encoding/Compressing */
int Encode(CLSZZInfo *LSZZInfo)
{
	short  i, len, r, s, last_match_length;
	short  c;

	if ( LSZZInfo->Mode == FileMode ){
		//get file length
		//fseek(LSZZInfo->infile, 0, 2);
		//LSZZInfo->textsize = ftell(LSZZInfo->infile);
		LSZZInfo->textsize = filelength( fileno(LSZZInfo->infile) );
		if (fwrite(&(LSZZInfo->textsize), sizeof LSZZInfo->textsize, 1, LSZZInfo->outfile) < 1)
			return  1;

		if (LSZZInfo->textsize == 0)
			return  2;
		rewind(LSZZInfo->infile);
	} else {
		LSZZInfo->textsize = MemLength(&(LSZZInfo->inMemInfo));
		if (Memwrite(&(LSZZInfo->textsize), sizeof LSZZInfo->textsize, 1, &(LSZZInfo->outMemInfo)) == -2)
			return  1;

		if (LSZZInfo->textsize == 0)
			return  2;
	};
	LSZZInfo->textsize = 0;			/* rewind and rescan */
	StartHuff(LSZZInfo);
	InitTree(LSZZInfo);
	s = 0;
	r = N - F;
	for (i = s; i < r; i++)
		LSZZInfo->text_buf[i] = ' ';
	if ( LSZZInfo->Mode == FileMode )
		for (len = 0; len < F && (c = getc(LSZZInfo->infile)) != EOF; len++)
			LSZZInfo->text_buf[r + len] = (unsigned char) c;
	else
		for (len = 0; len < F && (c = Memgetc(&(LSZZInfo->inMemInfo))) != EOF; len++)
			LSZZInfo->text_buf[r + len] = (unsigned char) c;
	LSZZInfo->textsize = len;
	for (i = 1; i <= F; i++)
		InsertNode((short)(r - i), LSZZInfo);
	InsertNode(r, LSZZInfo);
	LSZZInfo->printcount = 0;
	do {
		if (LSZZInfo->match_length > len)
			LSZZInfo->match_length = len;
		if (LSZZInfo->match_length <= THRESHOLD) {
			LSZZInfo->match_length = 1;
			EncodeChar(LSZZInfo->text_buf[r], LSZZInfo);
		} else {
			EncodeChar((unsigned short)(255 - THRESHOLD + LSZZInfo->match_length), LSZZInfo);
			EncodePosition(LSZZInfo->match_position,LSZZInfo);
		}
		last_match_length = LSZZInfo->match_length;
		if ( LSZZInfo->Mode == FileMode ){
			for (i = 0; i < last_match_length &&
					(c = getc(LSZZInfo->infile)) != EOF; i++) {
				DeleteNode(s, LSZZInfo);
				LSZZInfo->text_buf[s] = (unsigned char) c;
				if (s < F - 1)
					LSZZInfo->text_buf[s + N] = (unsigned char) c;
				s = (s + 1) & (N - 1);
				r = (r + 1) & (N - 1);
				InsertNode(r, LSZZInfo);
			}
		} else {
			for (i = 0; i < last_match_length &&
					(c = Memgetc( &(LSZZInfo->inMemInfo) )) != EOF; i++) {
				DeleteNode(s, LSZZInfo);
				LSZZInfo->text_buf[s] = (unsigned char) c;
				if (s < F - 1)
					LSZZInfo->text_buf[s + N] = (unsigned char) c;
				s = (s + 1) & (N - 1);
				r = (r + 1) & (N - 1);
				InsertNode(r, LSZZInfo);
			}
		};
		if ((LSZZInfo->textsize += i) > LSZZInfo->printcount) {
			//printf("%12ld\r", LSZZInfo->textsize);
			LSZZInfo->printcount += 1024;
		}
		while (i++ < last_match_length) {
			DeleteNode(s, LSZZInfo);
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			if (--len) InsertNode(r, LSZZInfo);
		}
	} while (len > 0);
	EncodeEnd(LSZZInfo);
	//printf("input: %ld bytes\n", LSZZInfo->textsize);
	//printf("output: %ld bytes\n", LSZZInfo->codesize);
	//printf("output/input: %.3f\n", (double)LSZZInfo->codesize / LSZZInfo->textsize);

	return  0;
} //end of Encode()


//Decoding/Uncompressing
int Decode(CLSZZInfo *LSZZInfo)
{
	unsigned short    i, j, k, r;
	unsigned long int count;
	unsigned short    c;

	if ( LSZZInfo->Mode == FileMode ){
		if (fread(&(LSZZInfo->textsize), sizeof LSZZInfo->textsize, 1, LSZZInfo->infile) < 1)
			return  1;
	} else {
		if (Memread(&(LSZZInfo->textsize), sizeof LSZZInfo->textsize, 1, &(LSZZInfo->inMemInfo)) < 1)
			return  2;
	}

	if (LSZZInfo->textsize == 0)
		return  3;

	StartHuff(LSZZInfo);
	for (i = 0; i < N - F; i++)
		LSZZInfo->text_buf[i] = ' ';
	r = N - F;
	LSZZInfo->printcount = 0;
	for (count = 0; count < LSZZInfo->textsize; ) {
		c = /*( unsigned char )*/DecodeChar(LSZZInfo);
		if (c < 256) {
			if (LSZZInfo->Mode == FileMode)
				putc(c, LSZZInfo->outfile);
			else {
				Memputc(c, &(LSZZInfo->outMemInfo));
			}
			LSZZInfo->text_buf[r++] = (unsigned char)c;
			r &= (N - 1);
			count++;
		} else {
			i = (r - DecodePosition(LSZZInfo) - 1) & (N - 1);
			j = c - 255 + THRESHOLD;
			for (k = 0; k < j; k++) {
				c = LSZZInfo->text_buf[(i + k) & (N - 1)];
				if ( LSZZInfo->Mode == FileMode )
					putc(c, LSZZInfo->outfile);
				else {
					Memputc(c, &(LSZZInfo->outMemInfo));
				}
				LSZZInfo->text_buf[r++] = (unsigned char)c;
				r &= (N - 1);
				count++;
			}
		}
		if (count > LSZZInfo->printcount) {
			//printf("%12ld\r", count);
			LSZZInfo->printcount += 1024;
		}
	}
	LSZZInfo->decodesize = count;

	return  0;
	//printf("%12ld\n", count);

} //end of Decode()

//memory processing function
int Memgetc(CMemInfo *MemInfo)
{
	char    buf[4096];
	DWORD	dwcbBuffer, dwRetCode;
	TS_COM_PROPS    *lptsComProps;

	if( MemInfo->inFile ) {
		//read the packet
		return  getc(MemInfo->inFile);
	}

	if (MemInfo->pMem == NULL) return -2;

	if (MemInfo->CurPos == MemInfo->length) {
	    if( MemInfo->outFile && !(MemInfo->fEof) ) {

		//read a packet
		//write to file, so get bytes from communication

		lptsComProps = (TS_COM_PROPS *)buf;

		dwRetCode = SrvReadExchngBuf(MemInfo->lpExchBufInfo, buf, 4096);
		if( dwRetCode != TERR_SUCCESS ) {
			MemInfo->fEof = TRUE;
		}

		if( lptsComProps->lp == cmFTP_FILE_END ) {
			MemInfo->fEof = TRUE;
		}

		if( lptsComProps->lp == cmFTP_FILE_ERROR ) {
			MemInfo->fEof = TRUE;
		}

		dwcbBuffer = lptsComProps->len;
		memcpy(MemInfo->pMem, (LPVOID)(buf+sizeof(TS_COM_PROPS)), dwcbBuffer);
	    } else {
		return EOF;
	    }
	}

	return (unsigned char)MemInfo->pMem[MemInfo->CurPos++];
}

int Memputc(int c, CMemInfo *MemInfo)
{
	TS_COM_PROPS    *lptsComProps;
	char             buf[4096];

	if( MemInfo->outFile ) {
		return  putc(c, MemInfo->inFile);
	}

	if (MemInfo->pMem == NULL) return -2;
	if (MemInfo->CurPos == MemInfo->length) {
	    if( MemInfo->inFile ) {

		//send the packet
		//come from file, so send them with communication

		lptsComProps = (TS_COM_PROPS *)buf;
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->lp = cmFTP_FILE_OK;
		lptsComProps->leftPacket = '\x1';
		lptsComProps->endPacket = '\x1';

		memcpy((LPVOID)(buf+sizeof(TS_COM_PROPS)), MemInfo->pMem, MemInfo->length);
		SrvWriteExchngBuf(MemInfo->lpExchBufInfo, buf, MemInfo->length);

		MemInfo->CurPos = 0;
	    } else {
		return EOF;
	    }
	}

	return /*(unsigned char)*/MemInfo->pMem[MemInfo->CurPos++] = (unsigned char)c;
}

/*
int Memseek(CMemInfo *MemInfo, long offset, int origin)
{
	if (MemInfo->pMem == NULL) return -2;

	MemInfo->CurPos = MemInfo->CurPos + offset;
	if (MemInfo->CurPos > MemInfo->length)
		MemInfo->CurPos = MemInfo->length;
	else if (MemInfo->CurPos < 0)
		MemInfo->CurPos = 0;

	return 0;
}


long Memtell(CMemInfo *MemInfo)
{
	if (MemInfo->pMem == NULL) return -2;

	return MemInfo->CurPos;
}

void Memrewind(CMemInfo *MemInfo)
{
	MemInfo->CurPos = 0;
}
*/

size_t Memread(void *buffer, size_t size, size_t count, CMemInfo *MemInfo)
{
	long ireadsize;

	if ((buffer == NULL) || (MemInfo == NULL)) return -2;

	ireadsize = size * count;
	if (ireadsize + MemInfo->CurPos > MemInfo->length)
		ireadsize = MemInfo->length - MemInfo->CurPos;
	memcpy(buffer, (void *)(MemInfo->pMem + MemInfo->CurPos), ireadsize);
	MemInfo->CurPos += ireadsize;
	return ireadsize;
}

size_t Memwrite(const void *buffer, size_t size, size_t count, CMemInfo *MemInfo)
{
	long iwritesize;

	if ((buffer == NULL) || (MemInfo == NULL)) return -2;
	
	iwritesize = size * count;
	if (iwritesize + MemInfo->CurPos > MemInfo->length) 
		iwritesize = MemInfo->length - MemInfo->CurPos;
	memcpy((void *)(MemInfo->pMem + MemInfo->CurPos), buffer, iwritesize);
	MemInfo->CurPos += iwritesize;
	return iwritesize;
}


size_t MemLength(CMemInfo *MemInfo)
{
	if (MemInfo == NULL) return -2;

	if( MemInfo->inFile )
	    return  filelength( fileno(MemInfo->inFile) );

	return MemInfo->length;
}


//inside use function
void initialVar(CLSZZInfo *LSZZInfo)
{
	LSZZInfo->textsize = 0;
	LSZZInfo->codesize = 0;
	LSZZInfo->printcount = 0;
	LSZZInfo->getbuf = 0;
	LSZZInfo->getlen = 0;
	LSZZInfo->putbuf = 0;
	LSZZInfo->putlen = 0;
	LSZZInfo->match_position = 0;
	LSZZInfo->match_length = 0;
	LSZZInfo->code = 0;
	LSZZInfo->len = 0;
} //end of initialVar()


//*dstLen <-- memory size
//
__declspec(dllexport) int MemCompressIntoMem(char *dst, long *dstLen, char *src, long srcLen)
{
	//parameter availability check
	CLSZZInfo LSZZInfo;

	if ((dst == NULL) || (src == NULL) || \
			(srcLen == 0) || (*dstLen < srcLen))
		return -1;

	LSZZInfo.Mode = MemMode;

	LSZZInfo.inMemInfo.inFile = NULL;
	LSZZInfo.outMemInfo.inFile = NULL;
	LSZZInfo.inMemInfo.length = srcLen;
	LSZZInfo.outMemInfo.length = *dstLen;

	LSZZInfo.inMemInfo.pMem = src;
	LSZZInfo.outMemInfo.pMem = dst;
	initialVar(&LSZZInfo);

	if( Encode(&LSZZInfo) )
		return  1;

	*dstLen = LSZZInfo.codesize;

	return 0;
};

__declspec(dllexport) int MemDecompressIntoMem(char *dst, long *dstLen, char *src, long srcLen)
{
	//parameter availability check
	CLSZZInfo LSZZInfo;

	if ((dst == NULL) || (src == NULL) || \
			(srcLen == 0) || (*dstLen < srcLen))
		return -1;

	if (*dstLen < 3*srcLen)
		return -1;

	LSZZInfo.Mode = MemMode;

	LSZZInfo.inMemInfo.inFile = NULL;
	LSZZInfo.outMemInfo.inFile = NULL;

	LSZZInfo.inMemInfo.length = srcLen;
	LSZZInfo.outMemInfo.length = *dstLen;
	LSZZInfo.inMemInfo.pMem = src;
	LSZZInfo.outMemInfo.pMem = dst;
	initialVar(&LSZZInfo);
	if( Decode(&LSZZInfo) )
		return  1;

	*dstLen = LSZZInfo.codesize;

	return 0;
}

__declspec(dllexport) int FileCompressIntoFile(char *szinFileName, char *szoutFileName)
{
	//parameter availability check
	CLSZZInfo LSZZInfo;
	int       i = 0;

	if ((szinFileName == NULL) || (szoutFileName == NULL))
		return -1;

	LSZZInfo.Mode = FileMode;
	LSZZInfo.infile  = fopen(szinFileName, "rb");
	LSZZInfo.outfile = fopen(szoutFileName, "wb");
	initialVar(&LSZZInfo);
	if( Encode(&LSZZInfo) )
		i = 1;

	fclose(LSZZInfo.infile);
	fclose(LSZZInfo.outfile);

	return i;
};

__declspec(dllexport) int FileDecompressIntoFile(char *szinFileName, char *szoutFileName)
{
	//parameter availability check
	CLSZZInfo LSZZInfo;
	int       i = 0;

	if ((szinFileName == NULL) || (szoutFileName == NULL))
		return -1;

	LSZZInfo.Mode = FileMode;
	LSZZInfo.infile  = fopen(szinFileName, "rb");
	LSZZInfo.outfile = fopen(szoutFileName, "wb");
	initialVar(&LSZZInfo);

	if( Decode(&LSZZInfo) )
		i = 1;

	fclose(LSZZInfo.infile);
	fclose(LSZZInfo.outfile);

	return  i;
}


//*dstLen <-- memory size
//
__declspec(dllexport) int FileCompressToSend(LPSTR lpFileName,
					     void *lpExchBufInfo)
{
	//parameter availability check
	CLSZZInfo LSZZInfo;
	char      buf[4096];
	char	  buff[4096];
	TS_COM_PROPS    *lptsComProps;
	DWORD	  dwRetCode;

	lptsComProps = (TS_COM_PROPS *)buf;

	LSZZInfo.Mode = MemMode;

	LSZZInfo.inMemInfo.inFile = fopen(lpFileName, "rb");
	if( LSZZInfo.inMemInfo.inFile == NULL ) {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->lp = GetLastError();

		dwRetCode = SrvWriteExchngBuf( lpExchBufInfo, buf, 4096 );
		return 1;
	}

	LSZZInfo.outMemInfo.inFile = LSZZInfo.inMemInfo.inFile;
	LSZZInfo.outMemInfo.outFile = LSZZInfo.inMemInfo.outFile = NULL;

	LSZZInfo.outMemInfo.lpExchBufInfo = lpExchBufInfo;
	LSZZInfo.inMemInfo.fEof = FALSE;
	LSZZInfo.outMemInfo.fEof = FALSE;

	LSZZInfo.outMemInfo.length = MAX_PKG_MSG_LEN;

	LSZZInfo.inMemInfo.pMem = NULL;
	LSZZInfo.outMemInfo.pMem = buff;
	initialVar(&LSZZInfo);

	if( Encode(&LSZZInfo) ) {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->lp = cmFTP_FILE_ERROR;

		dwRetCode = SrvWriteExchngBuf( lpExchBufInfo, buf, 4096 );
		return  1;
	}

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->len = (short)(LSZZInfo.outMemInfo.length);
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_END;

	memcpy((LPVOID)(buf+sizeof(TS_COM_PROPS)), buff, LSZZInfo.outMemInfo.length);

	SrvWriteExchngBuf(lpExchBufInfo, buf, LSZZInfo.outMemInfo.length);

	return  LSZZInfo.codesize;

} //FileCompressToSend()



__declspec(dllexport) int FileDecompressFromGet(LPSTR lpFileName,
						void *lpExchBufInfo)
{
	//parameter availability check
	CLSZZInfo LSZZInfo;
	char      buf[4096];
	char	  buff[4096];
	TS_COM_PROPS    *lptsComProps;
	DWORD	  dwRetCode;

	lptsComProps = (TS_COM_PROPS *)buf;

	LSZZInfo.Mode = MemMode;

	LSZZInfo.inMemInfo.outFile = fopen(lpFileName, "wb");
	if( LSZZInfo.inMemInfo.outFile == NULL ) {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->lp = GetLastError();

		dwRetCode = SrvWriteExchngBuf(lpExchBufInfo, buf, 4096);
		return 1;
	}
	fseek(LSZZInfo.inMemInfo.outFile, 0, SEEK_SET);

	LSZZInfo.outMemInfo.outFile = LSZZInfo.inMemInfo.outFile;
	LSZZInfo.outMemInfo.inFile = LSZZInfo.inMemInfo.inFile = NULL;

	LSZZInfo.outMemInfo.lpExchBufInfo = lpExchBufInfo;
	LSZZInfo.inMemInfo.fEof = FALSE;
	LSZZInfo.outMemInfo.fEof = FALSE;

	LSZZInfo.outMemInfo.length = MAX_PKG_MSG_LEN;

	LSZZInfo.inMemInfo.pMem = buff;
	LSZZInfo.outMemInfo.pMem = NULL;
	initialVar(&LSZZInfo);

	if( Decode(&LSZZInfo) ) {
		lptsComProps->packetType = pkTS_PIPE_FTP;
		lptsComProps->msgType = msgTS_FTP_FILE;
		lptsComProps->lp = cmFTP_FILE_ERROR;

		dwRetCode = SrvWriteExchngBuf( lpExchBufInfo, buf, 4096 );
		return  1;
	}

	ZeroMemory( (LPVOID)lptsComProps, sizeof(TS_COM_PROPS) );
	lptsComProps->len = (short)(LSZZInfo.outMemInfo.length);
	lptsComProps->packetType = pkTS_PIPE_FTP;
	lptsComProps->msgType = msgTS_FTP_FILE;
	lptsComProps->lp = cmFTP_FILE_END;

	memcpy((LPVOID)(buf+sizeof(TS_COM_PROPS)), buff, LSZZInfo.outMemInfo.length);

	SrvWriteExchngBuf(lpExchBufInfo, buf, LSZZInfo.outMemInfo.length);

	return  LSZZInfo.codesize;

} //end of FileDecompressFromGet()

