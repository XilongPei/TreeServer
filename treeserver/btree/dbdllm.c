/*****************
* dbdllm.c for DBTREE.C   98
*
****************************************************************************/
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinsrDll, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL fOK = TRUE;
    switch( fdwReason) {
	case DLL_PROCESS_ATTACH:
	    //DLL is attaching to the address space of current process.
	    //Increment this module's usage count when it is attached to a process
	break;
	case DLL_THREAD_ATTACH:
	    //A new thread is being created in the current process
	break;
	case DLL_THREAD_DETACH:
	    //A thread is exiting cleanly.
	break;
	case DLL_PROCESS_DETACH:
	    //The calling process is detaching the DLL from its address space.
	break;
    }
    return  fOK;
} //end of DllMain()
