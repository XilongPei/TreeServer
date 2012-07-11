{*************************************************************************}
{    Module Name:                                                         }
{        UsrRule.pas                                                      }
{    Abstract:                                                            }
{        TreeServer User account templete dialog.                         }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{*************************************************************************}

unit UsrRule;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Spin, ExtCtrls;

type
  TFrmUsrRule = class(TForm)
    GrpBoxMod: TGroupBox;
    GrpBoxRule: TGroupBox;
    BtnSure: TButton;
    BtnCancel: TButton;
    spedLFCnt: TSpinEdit;
    lbLFCnt: TLabel;
    ChkBoxLock: TCheckBox;
    edDefDir: TEdit;
    lbDefDir: TLabel;
    PnlAcsRight: TPanel;
    Label3: TLabel;
    ChkBoxDataAcs: TCheckBox;
    ChkBoxASQLAcs: TCheckBox;
    ChkBoxSaveAcs: TCheckBox;
    ChkBoxUsrAcs: TCheckBox;
    BtnDefTime: TButton;
    procedure BtnCancelClick(Sender: TObject);
    procedure BtnSureClick(Sender: TObject);
    procedure BtnDefTimeClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure edDefDirMChange(Sender: TObject);
    procedure ChkBoxLockClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  FrmUsrRule: TFrmUsrRule;

implementation

uses clockctl, PipeStruct, AcuntCtl;

{$R *.DFM}

procedure TFrmUsrRule.BtnCancelClick(Sender: TObject);
begin
     FrmUsrRule.Close;
end;

procedure TFrmUsrRule.BtnSureClick(Sender: TObject);
var
   LFCnt : Byte;
   szDefDir : Array[0..255] of Char;
begin
     if ChkBoxDataAcs.Checked then
        ComUserInfo.szAccessMask[0] := #1
     else
        ComUserInfo.szAccessMask[0] := #0;

     if ChkBoxASQLAcs.Checked  then
        ComUserInfo.szAccessMask[1] := #1
     else
        ComUserInfo.szAccessMask[1] := #0;

     if ChkBoxSaveAcs.Checked then
        ComUserInfo.szAccessMask[2] := #1
     else
        ComUserInfo.szAccessMask[2] := #0;

     if ChkBoxUsrAcs.Checked then
        ComUserInfo.szAccessMask[3] := #1
     else
        ComUserInfo.szAccessMask[3] := #0;

     LFCnt := spedLFCnt.Value;
     CopyMemory ( @ComUserInfo.cLogonFail, @LFCnt, 1 );

     if ChkBoxLock.Checked then
        ComUserInfo.cLocked := #1
     else
         ComUserInfo.cLocked := #0;

     if edDefDir.Text <> '' then
     begin
          StrPCopy ( szDefDir, edDefDir.Text );
          CopyMemory ( @ComUserInfo.szHomeDir[0], @szDefDir[0], StrLen ( szDefDir ) + 1);
     end;

     if not MdfyAcntRule ( ClihPipe, ComUserInfo ) then
     begin
          MessageBox ( 0, '修改用户规则失败！', 'TreeServer 用户管理器', MB_OK or MB_ICONSTOP );
          exit;
     end;

     FrmUsrRule.Close;
end;

procedure TFrmUsrRule.BtnDefTimeClick(Sender: TObject);
begin
     FrmClock.ShowModal;
end;

procedure TFrmUsrRule.FormShow(Sender: TObject);
var
   LFCnt : Byte;
   szDir : Array [0..MAX_PATH - 1] of Char;
begin
     with ComUserInfo do
     begin
          if szAccessMask[0] = #1 then
             ChkBoxDataAcs.Checked := True
          else
              ChkBoxDataAcs.Checked := False;

          if szAccessMask[1] = #1  then
             ChkBoxASQLAcs.Checked := True
          else
              ChkBoxASQLAcs.Checked := False;

          if szAccessMask[2] = #1 then
             ChkBoxSaveAcs.Checked := True
          else
              ChkBoxUsrAcs.Checked := False;

          if szAccessMask[3] = #1 then
             ChkBoxUsrAcs.Checked := True
          else
              ChkBoxUsrAcs.Checked := False;

          CopyMemory ( @LFCnt, @ComUserInfo.cLogonFail, 1 );
          spedLFCnt.Value := LFCnt;

          if ComUserInfo.cLocked = #1 then begin
              ChkBoxLock.Checked := True;
              spedLFCnt.Enabled := True;
              lbLFCnt.Enabled := True;
          end
          else begin
              ChkBoxLock.Checked := False;
              spedLFCnt.Enabled := False;
              lbLFCnt.Enabled := False;
          end;

          StrCopy ( szDir, @ComUserInfo.szHomeDir[0] );

          edDefDir.Text := szDir;
     end;
end;

procedure TFrmUsrRule.edDefDirMChange(Sender: TObject);
begin
     exit;
end;

procedure TFrmUsrRule.ChkBoxLockClick(Sender: TObject);
begin
    if ChkBoxLock.Checked then begin
       spedLFCnt.Enabled := True;
       lbLFCnt.Enabled := True;
    end
    else begin
       spedLFCnt.Enabled := False;
       spedLFCnt.Value := 0;
       lbLFCnt.Enabled := False;
    end;
end;

end.
