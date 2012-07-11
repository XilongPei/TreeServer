{*************************************************************************}
{    Module Name:                                                         }
{        adduser.pas                                                      }
{    Abstract:                                                            }
{        TreeServer User Manager add account window.                      }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{                                                                         }
{    Revision                                                             }
{        Niu Jingyu    1998.07.16                                         }
{            Format the error message.                                    }
{            Rewrite some code ( All rewrite code was marked ).           }
{*************************************************************************}

unit adduser;

interface
{$A-}

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, comctrls;

type
  TAddUserForm = class(TForm)
    lbUser: TLabel;
    lbDescription: TLabel;
    lbPassWord: TLabel;
    lbPassWordSure: TLabel;
    edUserName: TEdit;
    edDescription: TEdit;
    edPassword: TEdit;
    edPasswordSure: TEdit;
    lbHomeDir: TLabel;
    edHomeDir: TEdit;
    ChkBoxcbLocked: TCheckBox;
    BtnAccess: TButton;
    BtnLogon: TButton;
    BtnAppend: TButton;
    BtnCancel: TButton;
    ChkBoxcbCreateHome: TCheckBox;
    Button1: TButton;
    procedure BtnAppendClick(Sender: TObject);
    procedure BtnLogonClick(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure edHomeDirEnter(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure BtnAccessClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  AddUserForm: TAddUserForm;

implementation

{$R *.DFM}
uses main, PipeStruct, ClockCtl, LogonCtl, AcuntCtl, UsrRight, DesApi;

{ procedure TAddUserForm.BtnAppendClick rewrite by Niu Jingyu, 1998.07.16 }
procedure TAddUserForm.BtnAppendClick(Sender: TObject);
var
   ListItem : TlistItem;
   bAutoCrtHmDir : Boolean;
begin
    edHomeDir.OnEnter ( Sender );
    with ComUserInfo do
    begin
         if ( edUserName.Text = '' ) then begin
              MessageBox ( 0, '请输入用户名，用户名是帐号的关键字， 不能为空。', 'TreeServer 用户管理器', MB_Ok or MB_ICONINFORMATION );
              AddUserForm.ActiveControl := edUserName;
              exit;
         end;

         if ( edPassword.Text = '' ) and ( edPasswordSure.Text = '' ) then begin
              MessageBox ( 0, '为提高安全性，用户帐号的密码不能为空。', 'TreeServer 用户管理器', MB_Ok or MB_ICONINFORMATION );
              AddUserForm.ActiveControl := edPassword;
              exit;
         end;

         if ( edPassword.Text <> edPasswordSure.Text ) then
         begin
              MessageBox ( 0, '密码未被正确确认。请保证密码与确认密码完全相同。', 'TreeServer 用户管理器', MB_Ok or MB_ICONINFORMATION );
              edPassword.Clear;
              edPasswordSure.Clear;
              AddUserForm.ActiveControl := edPassword;
              exit;
         end;

         if ( edHomeDir.Text = '' ) then
         begin
              MessageBox ( 0, '必须为用户帐号指定宿主目录。', 'TreeServer 用户管理器', MB_Ok or MB_ICONINFORMATION );
              AddUserForm.ActiveControl := edHomeDir;
              exit;
         end;

         StrPCopy ( szUser, edUserName.Text );
         StrPCopy ( szDescription, edDescription.Text );
         DES ( pChar ( edPassWord.Text ), pChar ( edPassWord.Text ), @szPassWd[0] );
//         StrPCopy ( szPasswd, edPassword.Text );
         StrPCopy ( szHomeDir, edHomeDir.Text );

         if ChkBoxcbLocked.Checked then cLocked := #1
         else cLocked := #0;

         if ChkBoxcbCreateHome.Checked then
            bAutoCrtHmDir := True
         else
             bAutoCrtHmDir := False;
    end;

    if not UsrAccountAdd ( ClihPipe, ComUserInfo, bAutoCrtHmDir ) then
    begin
         MessageBox ( 0, '添加帐号失败!', 'TreeServer 用户管理器', MB_Ok or MB_ICONSTOP );
         exit;
    end;

    with TUserMgr.UserList do
    begin
         ListItem := Items.Add;
         ListItem.Caption := ComUserInfo.szUser;
         ListItem.SubItems.Add ( ComUserInfo.szDescription );
    end;

    AddUserForm.FormShow ( Sender );
    AddUserForm.BtnCancel.Caption := '关闭'; { Add By Niu Jingyu, 1998.07.16 }
end;

procedure TAddUserForm.BtnLogonClick(Sender: TObject);
begin
       FrmLogCtl.ShowModal;
end;

procedure TAddUserForm.Button1Click(Sender: TObject);
begin
     FrmClock.ShowModal;
end;

procedure TAddUserForm.edHomeDirEnter(Sender: TObject);
var
   szHmeDir : Array[0..MAX_PATH] of Char;
   szUsrNme : Array[0..MAX_USER_NAME - 1] of Char;
   HomeDir : String;
begin
     if  edUserName.Text <> '' then
     begin
          StrPCopy ( szUsrNme, edUserName.Text );
          StrPCopy ( szHmeDir, edHomeDir.Text );
          HomeDir := TransVDirToRDirU ( szHmeDir, szUsrNme );
          if HomeDir = '' then
             exit;
          edHomeDir.Text := HomeDir;
     end;
end;

procedure TAddUserForm.FormShow(Sender: TObject);
var
   HomeDir : String;
begin
     { Add By Niu Jingyu, 1998.07.16 }
     AddUserForm.BtnCancel.Caption := '取消';
     AddUserForm.BtnAppend.Default := True;
     AddUserForm.ActiveControl := edUserName;
     { End of add. }

     GetDefForUsrInfo ( ClihPipe, ComUserInfo );
     with AddUserForm do
     begin
          edUserName.Text := '';
          edDescription.Text := '';
          edPassword.Text := '';
          edPasswordSure.Text := '';

          HomeDir := TransVDirToRDirH( ClihPipe, ComUserInfo.szHomeDir );
          if HomeDir = '' then
             exit;
          edHomeDir.Text := HomeDir;

          ChkBoxcbCreateHome.Checked := False;
          ChkBoxcbLocked.Checked := False;
     end;
end;

procedure TAddUserForm.BtnAccessClick(Sender: TObject);
begin
     FrmUsrRight.ShowModal;
end;

end.
