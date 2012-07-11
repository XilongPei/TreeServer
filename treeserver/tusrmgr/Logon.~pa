{*************************************************************************}
{    Module Name:                                                         }
{        Logon.pas                                                        }
{    Abstract:                                                            }
{        TreeServer User Maganer Logon form.                              }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{                                                                         }
{    Revision                                                             }
{        Niu Jingyu    1998.07.15                                         }
{            Format the error message.                                    }
{        Wang Chao     1998.10.25                                         }
{            encrypt the password                                         }
{*************************************************************************}
unit Logon;

interface
{$A-}

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Buttons, ExtCtrls, Mask;

type
  TFrmLogon = class(TForm)
    edUsrName: TEdit;
    BtnSure: TBitBtn;
    BtnCancel: TBitBtn;
    lbUsrName: TLabel;
    lbPassWd: TLabel;
    MskedPassWd: TMaskEdit;
    Label1: TLabel;
    Image1: TImage;
    procedure BtnSureClick(Sender: TObject);
    procedure BtnCancelClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  FrmLogon: TFrmLogon;

implementation

uses main, AcuntCtl, PipeStruct, DesApi;

{$R *.DFM}

procedure TFrmLogon.BtnSureClick(Sender: TObject);
var
   szComputer : Array[ 0..MAX_COMPUTERNAME_LENGTH] of Char;
   Length : DWORD;
   szUsrname : Array[0..MAX_USER_NAME] of Char;
   szPasswd : Array[0..MAX_PASSWD - 1] of Char;
begin
     Length := MAX_COMPUTERNAME_LENGTH + 1;
     if edUsrName.Text = '' then
     begin
          MessageBox ( 0, '非法用户名，请输入正确的用户名！', '用户登录', MB_OK or MB_ICONINFORMATION );
          exit;
     end
     else
     begin
          StrPCopy ( szUsrName, edUsrName.Text );

//{$IFDEF  OldVersion}
          DES ( pChar ( MskedPassWd.Text ), pChar ( MskedPassWd.Text ), @szPasswd[0] );
//{$ELSE}
//          StrPCopy ( szPasswd, MskedPassWd.Text );
//{$ENDIF}

//          StrPCopy ( szPassWd, MskedPassWd.Text );
          if not GetComputerName( szComputer, Length ) then
          begin
               MessageBox ( 0, '无法得到当前计算机的名字！',
                          '用户登录', MB_Ok or MB_ICONSTOP );
               exit;
          end;
     end;

     ClihPipe := OpenNamedPipe ( '.' );

     if ClihPipe = INVALID_HANDLE_VALUE then
     begin
          MessageBox ( 0, PChar('打开 TreeServer 服务管道失败（'+IntToStr(GetLastError)+'）！'),
                     'TreeServer 用户管理器', MB_Ok or MB_ICONSTOP );
          exit;
     end;

     if TSLogon ( ClihPipe, szUsrName, szPasswd, szComputer ) = $FFFFFFFF then
     begin
          MessageBox ( 0, '用户登录失败！'+#13+#13+'检查输入的用户名和密码是否正确，然后重新登录。',
                     '用户登录', MB_Ok or MB_ICONSTOP );
          CloseTSNamedPipe ( ClihPipe );
          exit;
     end;

     FrmLogon.Visible := False;
     TUserMgr.ShowModal;
     FrmLogon.Close;
     FrmLogon.Release;  { Add by Niu Jingyu, 1998.07.15 }
end;

procedure TFrmLogon.BtnCancelClick(Sender: TObject);
begin
     FrmLogon.Close;
end;

end.
