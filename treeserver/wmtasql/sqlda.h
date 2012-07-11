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
 *
 *  Title:  sqlda.h
 *  Sccsid: @(#)sqlda.h 9.1.1.1 1/11/92  15:55:58
 *  Description:
 *      SQL Data Description Area
 *
 ***************************************************************************
 */

#pragma pack(push, 4)


#ifndef _SQLDA
#define _SQLDA

struct sqlvar_struct
	{
	short sqltype;      /* variable type        */
	short sqllen;       /* length in bytes      */
	char far *sqldata;      /* pointer to data      */
	short far *sqlind;      /* pointer to indicator     */
	char  far *sqlname;     /* variable name        */
	char  far *sqlformat;       /* reserved for future use  */
	short sqlitype;     /* ind variable type        */
	short sqlilen;      /* ind length in bytes      */
	char far *sqlidata;     /* ind data pointer     */
	};

struct sqlda
	{
	short sqld;
	struct sqlvar_struct far *sqlvar;
	char desc_name[19];     /* descriptor name      */
	short desc_occ;     /* size of sqlda structure  */
	struct sqlda far *desc_next;    /* pointer to next sqlda struct */
	};

#endif /* _SQLDA */

#pragma pack(pop, 4)

