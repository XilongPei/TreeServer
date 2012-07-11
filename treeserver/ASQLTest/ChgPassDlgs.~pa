unit ChgPassDlgs;

interface

uses Windows, SysUtils, Classes, Graphics, Forms, Controls, StdCtrls,
  Buttons, ExtCtrls, Messages;

type
  TChgPassDlg = class(TForm)
    Label1: TLabel;
    nameEdit: TEdit;
    Label2: TLabel;
    oldEdit: TEdit;
    Label3: TLabel;
    newEdit: TEdit;
    Label4: TLabel;
    new1Edit: TEdit;
    YesButton: TButton;
    NoButton: TButton;
    procedure FormActivate(Sender: TObject);
    procedure YesButtonClick(Sender: TObject);
  private
  public
  end;

var
  ChgPassDlg: TChgPassDlg;

implementation

uses MainForms, ASQLUnit;

{$R *.DFM}

procedure TChgPassDlg.FormActivate(Sender: TObject);
begin
  nameedit.text:=Mainuser;
end;

procedure TChgPassDlg.YesButtonClick(Sender: TObject);
begin
  oldedit.text:=trim(oldedit.text);
  newedit.text:=trim(newedit.text);
  new1edit.text:=trim(new1edit.text);
  if (oldedit.text='') then
  begin
    oldedit.setfocus;
    exit;
  end;

  if (newedit.text='') then
  begin
    newedit.setfocus;
    exit;
  end;

  if (new1edit.text='') then
  begin
    new1edit.setfocus;
    exit;
  end;

  if new1edit.text<>newedit.text then
  begin
    Application.MessageBox('新口令和确认新口令不一样', '提示框',
      MB_SYSTEMMODAL+mb_ok+MB_ICONEXCLAMATION);
    exit;
  end;
  if not Change_ASQL_Password(pchar(oldedit.text),pchar(newedit.text)) then
  begin
    Application.MessageBox('新口令更改没有成功', '提示框',
      MB_SYSTEMMODAL+mb_ok+MB_ICONEXCLAMATION);
    exit;
  end
  else
  begin
    Mainpassword:=newedit.text;
    modalresult:=mrok;
  end;
end;

end.
