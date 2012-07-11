{*************************************************************************}
{    Module Name:                                                         }
{        UsrRight.pas                                                     }
{    Abstract:                                                            }
{        TreeServer User rights information dialog.                       }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{*************************************************************************}

unit UsrRight;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls;

type
  TFrmUsrRight = class(TForm)
    GBoxUsrRight: TGroupBox;
    ChkBoxDataAcs: TCheckBox;
    ChkBoxASQLAcs: TCheckBox;
    ChkBoxSaveAcs: TCheckBox;
    ChkBoxUsrAcs: TCheckBox;
    BtnSure: TButton;
    BtnCancel: TButton;
    procedure BtnSureClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  FrmUsrRight: TFrmUsrRight;

implementation

{$R *.DFM}
uses Main, PipeStruct;

procedure TFrmUsrRight.BtnSureClick(Sender: TObject);
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
end;

procedure TFrmUsrRight.FormShow(Sender: TObject);
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
     end;
end;

end.
