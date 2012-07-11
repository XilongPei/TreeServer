/***************************************************************************
 *
 *             INFORMIX SOFTWARE, INC.
 *
 *                PROPRIETARY DATA
 *
 *  THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *  INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *  CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *  DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN AGREEMENT 
 *  SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *  THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *  SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *  UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *  Title:  blob.h
 *  Sccsid: @(#)blob.h  9.1.1.1 1/11/92  15:50:26
 *      (created from rsam/blob.h version 6.3)
 *
 *  Description:
 *              'blob.h' defines stuff for blobs
 *
 ***************************************************************************
 */

#pragma pack(push, 4)

#ifndef BLOB_DOT_H  /* To handle multiple includes */
#define BLOB_DOT_H

/*
 * Structure of the BlobLocation
 */
 

/*
 * blob definitions
 * tblob_t is the data that is stored in the tuple - it describes the blob
 * NOTE: tb_fd is expected to be the first member and TB_FLAGS gives offset
 *       to the tb_flags member.
 */

typedef struct tblob
	{
	int     tb_fd;      /* blob file descriptor (must be first) */
	int     tb_coloff;      /* Blob column offset in row        */
	long    tb_tblspace;    /* blob table space         */
	long    tb_start;   /* starting byte            */
	long    tb_end;     /* ending byte-0 for end of blob    */
	long        tb_size;        /* Size of blob                     */
	long        tb_addr;        /* Starting Sector or BlobPage      */
	long        tb_family;      /* Family ID                        */
	long        tb_volume;      /* Family Volume                    */
	int     tb_medium;      /* Medium - odd if removable        */
	int     tb_bstamp;      /* first BlobPage Blob stamp        */
	int     tb_sockid;  /* socket id of remote blob     */
	int     tb_flags;   /* flags - see below            */
	long    tb_reserved1;   /* reserved for the future      */
	long    tb_reserved2;   /* reserved for the future      */
	long    tb_reserved3;   /* reserved for the future      */
	long    tb_reserved4;   /* reserved for the future      */
	} tblob_t;

#ifdef OPTICAL
/**
 * for optical blobs, the system id (of the system where the blob was
 * created) is stored in the blob descriptor
 **/
#define tb_sysid        tb_reserved1
#endif /* OPTICAL */

#define TB_FLAGS        (2*INTSIZE+7*LONGSIZE+3*INTSIZE) 
#define SIZTBLOB        (11*LONGSIZE + 6*INTSIZE)

/* 'flags' definitions */
#define BLOBISNULL  (0x0001)    /* BLOB is NULL */
#define BLOBALIEN   (0x0002)    /* BLOB is ALIEN */
#define BL_BSBLOB       (0x0004)        /* blob is stored in blobspace */
#define BL_PNBLOB       (0x0008)        /* store in tablespace */
#define BL_DESCRIPTOR   (0x0010)    /* optical BLOB descriptor */

#ifdef OPTICAL
#define BL_SUBBLOB      (0x0010)        /* store in Optical Subsystem */
#define BL_BLOBID       (0x0020)        /* transfer the tblob (blob id) */
#endif /* OPTICAL */

/*
 * this struture is used to pass "useful" information back to
 * the user.
 */

typedef struct blobinfo
	{
	long    bi_size;        /* Size of blob         */
	long    bi_addr;        /* Starting Sector or BlobPage  */
	long    bi_family;      /* Family ID            */
	long    bi_volume;      /* Family Volume        */
	short   bi_flags;       /* flags            */
	short   bi_medium;      /* Medium - odd if removable    */
	} blobinfo_t;

#endif  /* BLOB_DOT_H : To handle multiple includes */

#pragma pack(pop, 4)
