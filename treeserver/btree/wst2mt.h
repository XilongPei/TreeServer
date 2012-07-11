/****************
* wst2mt.h
* Xilong Pei 1997.10
*****************************************************************************/

#ifndef __WST2MT_H_
#define __WST2MT_H_

//if the program is single thread, just undefine WSToMT
//#undef WSToMT
#define WSToMT	_declspec(thread)

#ifdef WSToMT
extern long lServerAsRunning;
extern WSToMT long treeSvrTaskId;
extern WSToMT int  intOfThread; 
extern WSToMT unsigned long callThreadId;
extern WSToMT unsigned long wmtExbuf;
#endif

#endif
