{*************************************************************************}
{    Module Name:                                                         }
{        Main.pas                                                         }
{    Abstract:                                                            }
{        TreeServer User Maganer main window.                             }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{                                                                         }
{    Revision                                                             }
{        Niu Jingyu    1998.07.15                                         }
{            Format the error message.                                    }
{            Add service status monitor.                                  }
{*************************************************************************}

unit main;

interface
{$A-}

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  Menus, ComCtrls, PipeStruct, StdCtrls, TSSvc, WinSvc, ExtCtrls, ImgList;

type
  TTUserMgr = class(TForm)
    TUMgrMenu: TMainMenu;
    muUser: TMenuItem;
    muView: TMenuItem;
    muOptions: TMenuItem;
    muHelp: TMenuItem;
    muNew: TMenuItem;
    N1: TMenuItem;
    muDelete: TMenuItem;
    muRen: TMenuItem;
    muProperty: TMenuItem;
    N2: TMenuItem;
    muExit: TMenuItem;
    UserList: TListView;
    LargeImages: TImageList;
    SmallImages: TImageList;
    muLargeIcon: TMenuItem;
    muSmallIcon: TMenuItem;
    muList: TMenuItem;
    muDetail: TMenuItem;
    StatusBar: TStatusBar;
    MuAcntRule: TMenuItem;
    N4: TMenuItem;
    N3: TMenuItem;
    T1: TMenuItem;
    N5: TMenuItem;
    A1: TMenuItem;
    Timer1: TTimer;
    N6: TMenuItem;
    list1: TMenuItem;
    procedure FormResize(Sender: TObject);
    procedure muNewClick(Sender: TObject);
    procedure muPropertyClick(Sender: TObject);
    procedure muRenClick(Sender: TObject);
    procedure muDeleteClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure mViewItemClick(Sender: TObject);
    procedure UserListDblClick(Sender: TObject);
    procedure muExitClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormShow(Sender: TObject);
    procedure muUserClick(Sender: TObject);
    procedure MuAcntRuleClick(Sender: TObject);
    procedure N4Click(Sender: TObject);
    procedure A1Click(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure list1Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    procedure ShowHint( Sender: TObject );
  end;
  function  TUserEnumProc( lpUserInfo : PTUserInfo ) : Boolean;

var
  TUserMgr: TTUserMgr;

implementation

uses adduser, prop, chngname, PipeFunc, AcuntCtl, UsrRule, about, listlog;

{$R *.DFM}

procedure TTUserMgr.ShowHint( Sender: TObject );
begin
    TUserMgr.StatusBar.Panels.Items[0].Text := ' ' + Application.Hint;
end;

procedure TTUserMgr.FormCreate(Sender: TObject);
var
  szComputerName: array[0..35] of char;
  dwcbName: DWORD;
begin
    dwcbName := 36;
    GetComputerName( szComputerName, dwcbName );

    TUserMgr.Caption := TUserMgr.Caption + ' - \\' + szComputerName;
    Application.OnHint := ShowHint;
    TUserMgr.StatusBar.Panels.Items[0].Text := ' 就绪'
end;

procedure TTUserMgr.FormResize(Sender: TObject);
begin
    UserList.Columns.Items[0].Width := UserList.Width div 3;
    UserList.Columns.Items[1].Width := UserList.Width -
            UserList.Columns.Items[0].Width - 4;
end;

procedure TTUserMgr.muNewClick(Sender: TObject);
begin
     AddUserForm.ShowModal;
end;

procedure TTUserMgr.muPropertyClick(Sender: TObject);
var
   szUsrNme : Array[ 0..MAX_USER_NAME] of Char;
begin
     GetDefForUsrInfo ( ClihPipe, ComUserInfo );

     if TUserMgr.UserList.Selected = nil then
         exit;

     StrPCopy ( szUsrNme, TUserMgr.UserList.Selected.Caption );

     if not ReceiveUsrAccountInfo ( ClihPipe, szUsrNme, ComUserInfo ) then
     begin
          MessageBox ( 0, '获取用户信息失败！', 'TreeServer 用户管理器', MB_Ok or MB_ICONSTOP );
          exit;
     end;

     UserPropForm.ShowModal;
end;

procedure TTUserMgr.muRenClick(Sender: TObject);
var
   szUsrNme : Array[ 0..MAX_USER_NAME] of Char;
begin
     if TUserMgr.UserList.Selected = nil then
         exit;

     StrPCopy ( szUsrNme, TUserMgr.UserList.Selected.Caption );

     { Add by Niu Jingyu, 1998.07.16 }
     if StrIComp( szUsrNme, 'ADMIN' ) = 0 then begin
         MessageBox( TUserMgr.Handle, PChar('系统不允许将帐号 ADMIN 更名。'),
            PAnsiChar('TreeSVR 用户管理器'),
            MB_OK or MB_ICONASTERISK );
         exit;
     end;

     if not ReceiveUsrAccountInfo ( ClihPipe, szUsrNme, ComUserInfo ) then
     begin
          MessageBox ( 0, '无法获取用户信息！', 'TreeServer 用户管理器', MB_Ok or MB_ICONSTOP );
          exit;
     end;

     ChngNameForm.ShowModal;
end;

procedure TTUserMgr.muDeleteClick(Sender: TObject);
var
    DelMessage: AnsiString;
    szUsrNme : Array[0..MAX_USER_NAME - 1] of Char;
    bAutoDelHomeDir : Boolean;
begin
     if TUserMgr.UserList.Selected = nil then
         exit;

     StrPCopy ( szUsrNme, TUserMgr.UserList.Selected.Caption );
     if StrIComp( szUsrNme, 'ADMIN' ) = 0 then begin
         DelMessage := 'Admin 帐号是 TreeSVR 的管理帐号，帐号具有对';
         DelMessage := DelMessage + ' TreeSVR 进行管理的权力，一旦该帐号删除，';
         DelMessage := DelMessage + '日后即使创建同名的帐号，仍无法行使对 TreeSVR';
         DelMessage := DelMessage + '的管理权力, 因此禁止删除该帐号。';
         MessageBox( TUserMgr.Handle, PAnsiChar(DelMessage),
            PAnsiChar('TreeSVR 用户管理器'),
            MB_OK or MB_ICONASTERISK );
         exit;
     end;

     DelMessage := '每个用户帐号皆有一个唯一的识别代号所代表，且该识别号';
     DelMessage := DelMessage + '独立于该用户名。一旦该帐号删除，日后即使';
     DelMessage := DelMessage + '创建同名的帐号，仍无法还原对资源的访问。';
     DelMessage := DelMessage + #13 + #13 + '是否确认删除该帐号？';
     if MessageBox( TUserMgr.Handle, PAnsiChar(DelMessage),
            PAnsiChar('TreeSVR 用户管理器'),
            MB_OKCANCEL or MB_ICONQUESTION ) = 2 then
     begin
          exit;
     end;

     if MessageBox ( 0, '是否自动删除用户主目录?',
                     'TreeServer 用户管理器', MB_OkCANCEL or MB_ICONQUESTION ) = IDOK then
        bAutoDelHomeDir := True
     else
        bAutoDelHomeDir := False;

     if not UsrAccountDelete ( ClihPipe, szUsrNme , bAutoDelHomeDir ) then
     begin
          MessageBox ( 0, '删除该用户帐号失败！', 'TreeServer 用户管理器', MB_Ok or MB_ICONSTOP );
          exit;
     end;

     if not GetDefForUsrInfo ( ClihPipe, ComUserInfo ) then
     begin
          exit;
     end;
     UserList.Items.Delete ( UserList.Selected.Index );
end;

procedure TTUserMgr.mViewItemClick(Sender: TObject);
var
  SenderName: String;
begin
     SenderName := TMenuItem(Sender).Name;
     TMenuItem(Sender).Checked := True;

     if SenderName = muLargeIcon.Name then begin
         UserList.ViewStyle := vsIcon;
     end
     else if SenderName = muSmallIcon.Name then begin
         UserList.ViewStyle := vsSmallIcon;
     end
     else if SenderName = muList.Name then begin
         UserList.ViewStyle := vsList;
     end
     else if SenderName = muDetail.Name then begin
         UserList.ViewStyle := vsReport;
     end;
end;

procedure TTUserMgr.UserListDblClick(Sender: TObject);
var
   szUsrNme : Array[ 0..MAX_USER_NAME] of Char;
begin
     GetDefForUsrInfo ( ClihPipe, ComUserInfo );

     if TUserMgr.UserList.Selected = nil then
         exit;

     StrPCopy ( szUsrNme, TUserMgr.UserList.Selected.Caption );

     if not ReceiveUsrAccountInfo ( ClihPipe, szUsrNme, ComUserInfo ) then
     begin
          MessageBox ( 0, '获取用户信息失败！', 'TreeServer 用户管理器', MB_Ok or MB_ICONSTOP );
          exit;
     end;

     UserPropForm.ShowModal;
end;

procedure TTUserMgr.muExitClick(Sender: TObject);
begin
     TUserMgr.Close;
end;

procedure TTUserMgr.FormClose(Sender: TObject; var Action: TCloseAction);
begin
     if not TsLogOff ( ClihPipe ) then
     begin
          MessageBox( 0, '注销用户失败。', 'TreeSVR 用户管理器', MB_OK or MB_ICONSTOP );
     end;

     CloseTSNamedPipe ( ClihPipe );
end;

function  TUserEnumProc( lpUserInfo : PTUserInfo ) : Boolean;
var
   TempListItem : TListItem;
begin
    TempListItem := TUserMgr.UserList.Items.Add;
    TempListItem.Caption := lpUserInfo.szUser;
    TempListItem.SubItems.Add( lpUserInfo.szDescription );
    Result := True;
end;

procedure TTUserMgr.FormShow(Sender: TObject);
begin
     EnumUsrAccount ( ClihPipe, TUserEnumProc );
end;

procedure TTUserMgr.muUserClick(Sender: TObject);
begin
     if UserList.Selected = nil then
     begin
          muProperty.Enabled := False;
          muRen.Enabled := False;
          muDelete.Enabled := False;
     end
     else
     begin
          muProperty.Enabled := True;
          muRen.Enabled := True;
          muDelete.Enabled := True;
     end;
end;

procedure TTUserMgr.MuAcntRuleClick(Sender: TObject);
begin
     RcvAcntRule ( ClihPipe, ComUserInfo );
     FrmUsrRule.ShowModal;
end;

procedure TTUserMgr.N4Click(Sender: TObject);
begin
     UserList.Items.Clear;
     EnumUsrAccount ( ClihPipe, TUserEnumProc );
end;

procedure TTUserMgr.A1Click(Sender: TObject);
begin
     AboutBox.ShowModal;
end;

{ procedure TTUserMgr.Timer1Timer Add by Niu Jingyu. 1998.07.16 }
{ Service status monitor. }
procedure TTUserMgr.Timer1Timer(Sender: TObject);
var
  dwRetCode: DWORD;
  dwStat: DWORD;
begin

{$IFDEF  OldVersion}
    { Query the TreeServer Service's status. }
    dwRetCode := TSServiceQuery( dwStat );
    if dwRetCode <> 0 then begin
        if MessageBox( 0, 'TreeServer 服务处于未知状态，用户管理器不能脱机工作。是否关闭用户管理器？',
                   'TreeServer 用户管理器', MB_OKCANCEL or MB_ICONQUESTION ) = IDOK then
            ExitProcess( 0 );
        Timer1.Enabled := FALSE;
        exit;
    end;

    if dwStat <> SERVICE_RUNNING then begin
        if MessageBox( 0, 'TreeServer 服务被停止，用户管理器不能脱机工作。是否关闭用户管理器？',
                    'TreeServer 用户管理器', MB_OKCANCEL or MB_ICONQUESTION ) = IDOK then
            ExitProcess( 0 );
        Timer1.Enabled := FALSE;
        exit;
    end;
{$ENDIF}

end;

procedure TTUserMgr.list1Click(Sender: TObject);
begin
    form1.showmodal;
end;

end.
