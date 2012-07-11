#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#define MYWM_NOTIFYICON (WM_APP+100)


HINSTANCE hInst;
HWND hSvrDlg;
int nServerStatus;
HANDLE hSTANDALONEMutex;

BOOL HandleNotifyIcon(HWND hDlg,UINT message);
void StateChange(HWND hDlg);
void NotifyDelete(HWND hDlg);
void NotifyAdd(HWND hDlg);
void Dlg_OnPaint(HWND hwnd);
int GetStatusIcon(void);
void DlgShowMsg(HWND hDlg,LPCTSTR lpString);
void ChangeMenuItem(HMENU hMenu);
BOOL APIENTRY AboutProc(HWND hDlg,UINT message,
						WPARAM wParam,LPARAM lParam);

