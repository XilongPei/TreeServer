/****************
* wst2mt.c
* Xilong Pei 1997.10
*****************************************************************************/

#include "wst2mt.h"

#ifdef WSToMT
long lServerAsRunning = 1;
WSToMT long treeSvrTaskId = -1;
WSToMT int  intOfThread = -1;
WSToMT unsigned long callThreadId;
WSToMT unsigned long wmtExbuf;

#endif
