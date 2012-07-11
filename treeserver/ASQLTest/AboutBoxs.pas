unit AboutBoxs;

interface

uses Windows, SysUtils, Classes, Graphics, Forms, Controls, StdCtrls,
  Buttons, ExtCtrls, Messages;

type
  TAboutDlg = class(TForm)
    Copyright: TLabel;
    Bevel1: TBevel;
    SKUName: TLabel;
    Image1: TImage;
    YkxtLabel1: TLabel;
    YkxtLabel2: TLabel;
    Label1: TLabel;
    Label2: TLabel;
    BitBtn1: TButton;
    procedure OKButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure BitBtn1MouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure BitBtn1MouseUp(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure BitBtn1MouseMove(Sender: TObject; Shift: TShiftState; X,
      Y: Integer);
  private
  public
    easter_egg:boolean;
  end;

var
  AboutDlg: TAboutDlg;

implementation

uses MainForms;

{$R *.DFM}

procedure TAboutDlg.OKButtonClick(Sender: TObject);
begin
    close;
end;

procedure TAboutDlg.FormCreate(Sender: TObject);
begin
  YkxtLabel1.caption:=Application.Title;
  YkxtLabel2.caption:=Application.Title;
  easter_egg:=false;
end;

procedure TAboutDlg.BitBtn1MouseDown(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  easter_egg:=true;
end;

procedure TAboutDlg.BitBtn1MouseUp(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  easter_egg:=false;
end;

procedure TAboutDlg.BitBtn1MouseMove(Sender: TObject; Shift: TShiftState;
  X, Y: Integer);

begin
  if easter_egg then
  begin
    ReleaseCapture;
    (Sender as TWinControl).perform(WM_SYSCOMMAND,$f012,0);
    easter_egg:=false;
  end;
end;

end.
