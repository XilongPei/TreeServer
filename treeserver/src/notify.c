#include "Notify.h"
#include "resource.h"

void DlgShowMsg(HWND hDlg, LPCTSTR lpString)
{
	SetDlgItemText( hDlg, IDC_STATELINE, lpString );
} 

void ChangeMenuItem(HMENU hMenu)
{
	EnableMenuItem(hMenu, 
		IDC_START,
		(nServerStatus==1)||(nServerStatus==2) ?MF_ENABLED:(MF_DISABLED|MF_GRAYED));
	EnableMenuItem(hMenu, 
		IDC_STOP,
		nServerStatus==0 ?MF_ENABLED:(MF_DISABLED|MF_GRAYED));
	EnableMenuItem(hMenu, 
		IDC_PAUSE,
		nServerStatus==0 ?MF_ENABLED:(MF_DISABLED|MF_GRAYED));
}
 
BOOL HandleNotifyIcon(HWND hDlg,UINT message)
{
	BOOL res;
	NOTIFYICONDATA tnid;

	tnid.cbSize=sizeof(NOTIFYICONDATA);
	tnid.hWnd=hDlg;
	tnid.uID=IDI_ICONRUN;
	tnid.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
	tnid.uCallbackMessage=MYWM_NOTIFYICON;
	
	tnid.hIcon=LoadIcon(hInst,MAKEINTRESOURCE( GetStatusIcon() ));
	lstrcpyn(tnid.szTip,
		"TBroker v5.0 ·þÎñ",
		sizeof(tnid.szTip));

	res=Shell_NotifyIcon(message,&tnid);

	if(tnid.hIcon)
		DestroyIcon(tnid.hIcon);

	return(res);
}

int GetStatusIcon(void)
{
	switch(nServerStatus)
	{
	case 0:
		return(IDI_ICONRUN);
	case 1:
		return(IDI_ICONSTOP);
	case 2:
		return(IDI_ICONPAUSE);
	default:
		return(IDI_ICONBLANK);
	}
}


void StateChange(HWND hDlg)
{
	HandleNotifyIcon(hDlg,NIM_MODIFY);
	EnableWindow(GetDlgItem(hDlg,IDC_START),(nServerStatus==1)||(nServerStatus==2) ?TRUE:FALSE);
	EnableWindow(GetDlgItem(hDlg,IDC_STOP),nServerStatus==0?TRUE:FALSE);
	EnableWindow(GetDlgItem(hDlg,IDC_PAUSE),nServerStatus==0?TRUE:FALSE);
	InvalidateRect(hDlg,NULL,FALSE);
}

void NotifyDelete(HWND hDlg)
{
	HandleNotifyIcon(hDlg,NIM_DELETE);
}

void NotifyAdd(HWND hDlg)
{
	HandleNotifyIcon(hDlg,NIM_ADD);
	StateChange(hDlg);
}

BOOL APIENTRY AboutProc(HWND hDlg,UINT message,
						WPARAM wParam,LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		if((LOWORD(wParam)==IDOK)||(LOWORD(wParam)==IDCANCEL))
		{
			EndDialog(hDlg,0);
			return(TRUE);
		}
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

void Dlg_OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HBITMAP hBitmap;
	HDC hMemdc;
	switch (nServerStatus)
	{
	case 0:
		hBitmap=LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BITMAPRUN));
		break;
	case 1:
		hBitmap=LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BITMAPSTOP));
		break;
	case 2:
		hBitmap=LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BITMAPPAUSE));
		break;
	default:
		hBitmap=LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BITMAPBLANK));
		break;
	}
	hMemdc=CreateCompatibleDC(NULL);
	SelectObject(hMemdc,hBitmap);

	BeginPaint(hwnd,&ps);

	BitBlt(ps.hdc,
		50,
		80,
		150,
		150,
		hMemdc,
		0,
		0,
		SRCCOPY);

	EndPaint(hwnd,&ps);
}	
