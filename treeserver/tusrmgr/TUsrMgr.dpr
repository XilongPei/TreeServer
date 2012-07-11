{*************************************************************************}
{    Module Name:                                                         }
{        TusrMgr.dpr                                                     }
{    Abstract:                                                            }
{        TreeServer User Maganer project source.                               }
{    Author:                                                              }
{        Niu Jingyu                                                       }
{    Copyright (c) NiuJingyu 1998  China Railway Software Corporation.    }
{*************************************************************************}

program TUsrMgr;

uses
  Forms,
  Windows,
  SysUtils,
  WinSvc,
  main in 'main.pas' {TUserMgr},
  adduser in 'adduser.pas' {AddUserForm},
  prop in 'prop.pas' {UserPropForm},
  chngname in 'chngname.pas' {ChngNameForm},
  PipeStruct in 'PipeStruct.pas',
  PipeFunc in 'PipeFunc.pas',
  clockctl in 'clockctl.pas' {FrmClock},
  ts_com in 'ts_com.pas',
  terror in 'terror.pas',
  AcuntCtl in 'AcuntCtl.pas',
  Logon in 'Logon.pas' {FrmLogon},
  logonCtl in 'logonCtl.pas' {FrmLogCtl},
  UsrRule in 'UsrRule.pas' {FrmUsrRule},
  UsrRight in 'UsrRight.pas' {FrmUsrRight},
  About in 'About.pas' {AboutBox},
  TSSvc in 'TSSvc.pas',
  listlog in 'listlog.pas' {Form1},
  DesApi in 'DesApi.pas';
var
  OSVersionInfo: TOSVersionInfo;
  dwRetCode: DWORD;
  dwStat: DWORD;

{$R *.RES}

begin
  Application.Initialize;

{$IFDEF  OldVersion}
  { Following source code add by Niu Jingyu. }
  { In order to check the platform and the OS version. }
  { This program only run on the Windows NT 4.0 and above version. }

  { Get the platform and OS information. }
  OSVersionInfo.dwOSVersionInfoSize := SizeOf( TOSVERSIONINFO );
  GetVersionEx( OSVersionInfo );
  { Application not running on the Windows NT. }
  if OSVersionInfo.dwPlatformId <> VER_PLATFORM_WIN32_NT then begin
      MessageBox( 0, 'TreeServer 用户管理器需要运行于 Micosoft Windows NT 4.00 或更高版本。',
                 'TreeServer 用户管理器', MB_OK or MB_ICONINFORMATION );
      ExitProcess( 0 );
  end;

  { Windows NT version must above 4.00. }
  if OSVersionInfo.dwMajorVersion < 4 then begin
      MessageBox( 0, 'TreeServer 用户管理器需要运行于 Micosoft Windows NT 4.00 或更高版本。',
                 'TreeServer 用户管理器', MB_OK or MB_ICONINFORMATION );
      ExitProcess( 0 );
  end;

  { Query the TreeServer Service's status. }
  dwRetCode := TSServiceQuery( dwStat );
  if dwRetCode <> 0 then begin
      if dwRetCode = ERROR_SERVICE_DOES_NOT_EXIST then begin
          MessageBox( 0, 'TreeServer 用户管理器需要同 TreeServer 运行于同一服务器，但本服务器没有安装 TreeServer 服务。',
                      'TreeServer 用户管理器', MB_OK or MB_ICONINFORMATION );
          ExitProcess( 0 );
      end
      else begin
          MessageBox( 0, PChar('查询 TreeServer 服务状态失败（'+IntToStr( GetLastError )+'）。'),
                      'TreeServer 用户管理器', MB_OK or MB_ICONSTOP );
          ExitProcess( 0 );
      end;
  end;

  if dwStat <> SERVICE_RUNNING then begin
      MessageBox( 0, 'TreeServer 服务没有运行，用户管理器不能脱机工作。',
                  'TreeServer 用户管理器', MB_OK or MB_ICONINFORMATION );
      ExitProcess( 0 );
  end;
  { End of the source code add by Niu Jingyu. }

{$ENDIF}

  Application.CreateForm(TFrmLogon, FrmLogon);
  Application.CreateForm(TTUserMgr, TUserMgr);
  Application.CreateForm(TAddUserForm, AddUserForm);
  Application.CreateForm(TUserPropForm, UserPropForm);
  Application.CreateForm(TChngNameForm, ChngNameForm);
  Application.CreateForm(TFrmClock, FrmClock);
  Application.CreateForm(TFrmLogCtl, FrmLogCtl);
  Application.CreateForm(TFrmUsrRule, FrmUsrRule);
  Application.CreateForm(TFrmUsrRight, FrmUsrRight);
  Application.CreateForm(TAboutBox, AboutBox);
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
