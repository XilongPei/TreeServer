/******************************
*	Module Name:
*		TRunInfo.H
*
*	Abstract:
*		Tree Server Runtime Info Map File and mutex
*       for Control Console use them 
*
*	Author:
*		Xilong Pei.
*
*	Copyright (c) 1999  China Railway Software Corporation.
*
*	Revision History:
*		Write by Xilong Pei, 1999.3
********************************************************************************/

#ifndef __TRunInfo_H__
#define __TRunInfo_H__

extern HANDLE	hTreeRunInfoMapFile;
extern LPSTR	lpTreeRunInfoMapAddress;
extern HANDLE	hTreeRunInfoConnectionMutex;

int CreateRunInfoMapForTreeSVR( void );
int freeRunInfoMapForTreeSVR( void );
int OpenRunInfoMapForTreeSVR( void );

#endif