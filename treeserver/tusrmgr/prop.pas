{*************************************************************************}
{    Module Name:                                                         }
{        prop.pas                                                         }
{    Abstract:                                                            }
{        TreeServer User Manager account properties window.               }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{                                                                         }
{    Revision                                                             }
{        Niu Jingyu    1998.07.16                                         }
{            Format the error message.                                    }
{            Rewrite some code ( All rewrite code was marked ).           }
{*************************************************************************}

unit prop;

interface
{$A-}

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Buttons, Spin;

type
  TUserPropForm = class(TForm)
    lbUserNameS: TLabel;
    lbDesc: TLabel;
    lbPassword: TLabel;
    lbPasswordSure: TLabel;
    lbHomeDir: TLabel;
    edDesc: TEdit;
    edPassword: TEdit;
    edPasswordSure: TEdit;
    edHomeDir: TEdit;
    ChkBoxLock: TCheckBox;
    Access: TButton;
    BtnLogon: TButton;
    lbUserNameD: TLabel;
    lbUserIdS: TLabel;
    lbUserIdD: TLabel;
    edChange: TButton;
    edCancel: TButton;
    BtnTimeCtl: TBitBtn;
    lbLFailCnt: TLabel;
    spedLFCnt: TSpinEdit;
    btnClear: TButton;
    lbLLogS: TLabel;
    lbLLogF: TLabel;
    lbLLogSV: TLabel;
    lbLLogFV: TLabel;
    procedure BtnLogonClick(Sender: TObject);
    procedure BtnTimeCtlClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure edChangeClick(Sender: TObject);
    procedure edCancelClick(Sender: TObject);
    procedure btnClearClick(Sender: TObject);
    procedure AccessClick(Sender: TObject);
    procedure edPasswordKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure edPasswordSureKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
  private
    { Private declarations }
  public
    { Public declarations }
  end;
  function WeekToStr( iDayOfWeek: Integer ): string;

var
  UserPropForm: TUserPropForm;

const
  passwdchar = '******************************';

implementation

uses logonCtl, clockctl, main, AcuntCtl, PipeStruct, UsrRight, DesApi;

{$R *.DFM}


var
   bPasswdChange, bPasswdSureChange : Boolean;

function WeekToStr( iDayOfWeek: Integer ): string;
begin
    case iDayOfWeek of
        0:  Result := '������';
        1:  Result := '����һ';
        2:  Result := '���ڶ�';
        3:  Result := '������';
        4:  Result := '������';
        5:  Result := '������';
        6:  Result := '������';
    else
        Result := '';
    end;
end;

procedure TUserPropForm.BtnLogonClick(Sender: TObject);
begin
     FrmLogCtl.ShowModal;
end;

procedure TUserPropForm.BtnTimeCtlClick(Sender: TObject);
begin
     FrmClock.ShowModal;
end;

procedure TUserPropForm.FormShow(Sender: TObject);
var
   bLgFail : Byte;
   sDate, sTime : String;
begin
     if  ( ComUserInfo.szUser = 'admin' ) or ( ComUserInfo.szUser = 'ADMIN' ) then
     begin
          ChkBoxLock.Enabled := False;
          BtnLogon.Enabled := False;
          BtnTimeCtl.Enabled := False;
     end
     else
     begin
          ChkBoxLock.Enabled := True;
          BtnLogon.Enabled := True;
          BtnTimeCtl.Enabled := True;
     end;

     with ComUserInfo do
     begin
          lbUserNameD.Caption := szUser;
          lbUserIdD.Caption := IntToStr( dwUserId );
          edDesc.Text := szDescription;
          //edPassword.Text := StrPas ( szPasswd );
          //edPasswordSure.Text := StrPas ( szPasswd );
          // Show mute password char in password box
          // Update bu Jingyu Niu, 1999.09
          edPassword.Text := Copy( passwdchar, 0, strlen( szPasswd ) );
          edPasswordSure.Text := edPassword.Text;
          edHomeDir.Text := szHomeDir;
          CopyMemory ( @bLgFail, @cLogonFail, 1 );
          spedLFCnt.Value := bLgFail;
          if cLocked = #1 then ChkBoxLock.Checked := True
          else ChkBoxLock.Checked := False;

          ShortDateFormat := 'yyyy-mm-dd';
          try
            begin
               if tLastLogon.Year <> 0 then begin
                  sDate := DateToStr ( EncodeDate( tLastLogon.Year, tLastLogon.Month, tLastLogon.Day ) );
                  sTime := TimeToStr ( EncodeTime( tLastLogon.Hour, tLastLogon.Min, tLastLogon.Sec, 0 ) );
                  lbLLogSV.Caption := sDate + ' ' + WeekToStr( tLastLogon.week ) + ' ' + sTime;
               end
               else
                  lbLLogSV.Caption := '��';
            end
          except
            begin
               lbLLogSV.Caption := '';
            end;
          end;

          try
            begin
               if tLastLogonFail.Year <> 0 then begin
                  sDate := DateToStr ( EncodeDate( tLastLogonFail.Year, tLastLogonFail.Month, tLastLogonFail.Day ) );
                  sTime := TImeToStr ( EncodeTime( tLastLogonFail.Hour, tLastLogonFail.Min, tLastLogonFail.Sec, 0 ) );
                  lbLLogFV.Caption := sDate + ' ' + WeekToStr( tLastLogonFail.week ) + ' ' + sTime;
               end
               else
                  lbLLogFV.Caption := '��';
            end
          except
            begin
               lbLLogFV.Caption := '';
            end;
          end;
     end;
end;

{ procedure TUserPropForm.edChangeClick Rewrite by Niu Jingyu, 1998.07.16 }
procedure TUserPropForm.edChangeClick(Sender: TObject);
var
   bLgFail : Byte;
begin
     if ( edPassword.Text = '' ) and ( edPasswordSure.Text = '' ) then begin
          MessageBox ( 0, 'Ϊ��߰�ȫ�ԣ��û��ʺŵ����벻��Ϊ�ա�', 'TreeServer �û�������', MB_Ok or MB_ICONINFORMATION );
          UserPropForm.ActiveControl := edPassword;
          exit;
     end;

     if ( edPassword.Text <> edPasswordSure.Text ) then
     begin
          MessageBox ( 0, '����δ����ȷȷ�ϡ��뱣֤������ȷ��������ȫ��ͬ��', 'TreeServer �û�������', MB_Ok or MB_ICONINFORMATION );
          edPassword.Clear;
          edPasswordSure.Clear;
          UserPropForm.ActiveControl := edPassword;
          exit;
     end;

     if ( edHomeDir.Text = '' ) then
     begin
          MessageBox ( 0, '����Ϊ�û��ʺ�ָ������Ŀ¼��', 'TreeServer �û�������', MB_Ok or MB_ICONINFORMATION );
          UserPropForm.ActiveControl := edHomeDir;
          exit;
     end;

     with ComUserInfo do
     begin
          StrPCopy ( szDescription, edDesc.Text );
          ////values of the two edits about password is cryptograph,
          ////so if users change the password they should change them completely
          if bPasswdChange or bPasswdSureChange then
          begin
               DES ( pChar ( edPassWord.Text ), pChar ( edPassWord.Text ), @szPasswd[0] );
               bPasswdChange := False;
               bPasswdSureChange := False;
          end
          else begin
              // Password not changed, do nothing
              // Update bu Jingyu Niu, 1999.09
              //StrPCopy ( szPasswd, edPassword.Text );
          end;

          StrPCopy ( szHomeDir, edHomeDir.Text );
          bLgFail := spedLFCnt.Value;
          CopyMemory ( @cLogonFail, @bLgFail, 1 );
          if ChkBoxLock.Checked then cLocked := #1
          else cLocked := #0;

     end;

     if not ModifyUsrAccountInfo ( ClihPipe, ComUserInfo ) then
     begin
          MessageBox ( 0, '�޸��û��ʺ���Ϣʧ�ܣ�', 'TreeServer �û�������', MB_Ok or MB_ICONSTOP );
          exit;
     end;

     TUserMgr.UserList.Selected.SubItems[0] := ComUserInfo.szDescription;

     UserPropForm.Close;
end;

procedure TUserPropForm.edCancelClick(Sender: TObject);
begin
     UserPropForm.Close;
end;

procedure TUserPropForm.btnClearClick(Sender: TObject);
begin
    spedLFCnt.Value := 0;
end;

procedure TUserPropForm.AccessClick(Sender: TObject);
begin
     FrmUsrRight.ShowModal;
end;

procedure TUserPropForm.edPasswordKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
     bPasswdChange := True;
end;

procedure TUserPropForm.edPasswordSureKeyDown(Sender: TObject;
  var Key: Word; Shift: TShiftState);
begin
     bPasswdSureChange := True;
end;

end.