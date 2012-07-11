unit VarDlgs;

interface

uses Windows, SysUtils, Classes, Graphics, Forms, Controls, StdCtrls, 
  Buttons, ExtCtrls;

type
  TVarDlg = class(TForm)
    Label1: TLabel;
    NameEdit: TEdit;
    Label2: TLabel;
    LxComboBox: TComboBox;
    Label3: TLabel;
    LenEdit: TEdit;
    YesButton: TButton;
    NoButton: TButton;
    Label4: TLabel;
    cszEdit: TEdit;
    procedure YesButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  private
  public
    varName,varcsz,Lx:string;
  end;

const
  LxTable:array [0..4,0..1] of string=(('字符串','1051'),('整数','1054'),
                                       ('长整数','1055'),('实数','1056'),
                                       ('字符','1053'));
var
  VarDlg: TVarDlg;

implementation

{$R *.DFM}

procedure TVarDlg.YesButtonClick(Sender: TObject);
begin
  NameEdit.text:=trim(NameEdit.text);
  LenEdit.text:=trim(LenEdit.text);
  if (LenEdit.text='') or (NameEdit.text='') then exit;
  try
    if strtoint(LenEdit.text)<0 then
    begin
      LenEdit.setfocus;
      exit;
    end;
  except
    LenEdit.setfocus;
    exit;
  end;
  Lx:=LxTable[LxComboBox.ItemIndex,0];
  varname:=nameedit.text;
  varcsz:=cszedit.text;
  Modalresult:=mrok;
end;

procedure TVarDlg.FormCreate(Sender: TObject);
var
  i:integer;
begin
  for i:=low(LxTable) to high(LxTable) do
    LxComboBox.Items.add(LxTable[i,0]);

  LxComboBox.ItemIndex:=0;
end;

end.
