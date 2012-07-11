/***************************************************************************
 *
 *			   INFORMIX SOFTWARE, INC.
 *
 *			      PROPRIETARY DATA
 *
 *	THIS DOCUMENT CONTAINS TRADE SECRET DATA WHICH IS THE PROPERTY OF 
 *	INFORMIX SOFTWARE, INC.  THIS DOCUMENT IS SUBMITTED TO RECIPIENT IN
 *	CONFIDENCE.  INFORMATION CONTAINED HEREIN MAY NOT BE USED, COPIED OR 
 *	DISCLOSED IN WHOLE OR IN PART EXCEPT AS PERMITTED BY WRITTEN AGREEMENT 
 *	SIGNED BY AN OFFICER OF INFORMIX SOFTWARE, INC.
 *
 *	THIS MATERIAL IS ALSO COPYRIGHTED AS AN UNPUBLISHED WORK UNDER
 *	SECTIONS 104 AND 408 OF TITLE 17 OF THE UNITED STATES CODE. 
 *	UNAUTHORIZED USE, COPYING OR OTHER REPRODUCTION IS PROHIBITED BY LAW.
 *
 *
 *  Title:	sqlstype.h
 *  Sccsid:	@(#)sqlstype.h	9.1.1.2	8/14/92  15:59:36
 *  Description:
 *		defined symbols for SQL statement types
 *
 ***************************************************************************
 */

#pragma pack(push, 4)

/*
 * SQL statement types
 */

#define SQ_DATABASE	1
#define SQ_SELECT	2	/* for internal use only */
#define SQ_SELINTO	3
#define SQ_UPDATE	4
#define SQ_DELETE	5
#define SQ_INSERT	6
#define SQ_UPDCURR	7
#define SQ_DELCURR	8
#define SQ_LDINSERT	9
#define SQ_LOCK		10
#define SQ_UNLOCK	11
#define SQ_CREADB	12
#define SQ_DROPDB	13
#define SQ_CRETAB	14
#define SQ_DRPTAB	15
#define SQ_CREIDX	16
#define SQ_DRPIDX	17
#define SQ_GRANT	18
#define SQ_REVOKE	19
#define SQ_RENTAB	20
#define SQ_RENCOL	21
#define SQ_CREAUD	22
#define SQ_STRAUD	23
#define SQ_STPAUD	24
#define SQ_DRPAUD	25
#define SQ_RECTAB	26
#define SQ_CHKTAB	27
#define SQ_REPTAB	28
#define SQ_ALTER	29
#define SQ_STATS	30
#define SQ_CLSDB	31
#define SQ_DELALL	32
#define SQ_UPDALL	33
#define SQ_BEGWORK	34
#define SQ_COMMIT	35
#define SQ_ROLLBACK	36
#define SQ_SAVEPOINT	37
#define SQ_STARTDB	38
#define SQ_RFORWARD	39
#define SQ_CREVIEW	40
#define SQ_DROPVIEW	41
#define SQ_DEBUG	42
#define SQ_CREASYN	43
#define SQ_DROPSYN	44
#define SQ_CTEMP	45
#define SQ_WAITFOR	46
#define SQ_ALTIDX       47
#define SQ_ISOLATE	48
#define SQ_SETLOG	49
#define SQ_EXPLAIN	50
#define SQ_SCHEMA	51
#define SQ_OPTIM	52
#define SQ_CREPROC	53
#define SQ_DRPPROC	54
#define SQ_CONSTRMODE   55
#define SQ_EXECPROC	56
#define SQ_DBGFILE	57
#define SQ_CREOPCL	58
#define SQ_ALTOPCL	59
#define SQ_DRPOPCL	60
#define SQ_OPRESERVE	61
#define SQ_OPRELEASE	62
#define SQ_OPTIMEOUT	63
#define SQ_PROCSTATS    64

#ifdef KANJI
#define SQ_GRANTGRP	65			/* JPN_ version only */
#define SQ_REVOKGRP	66			/* JPN_ version only */
#endif /* KANJI */

#define SQ_CRETRIG      70
#define SQ_DRPTRIG      71

#pragma pack(pop, 4)

