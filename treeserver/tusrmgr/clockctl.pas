{*************************************************************************}
{    Module Name:                                                         }
{        ClockCtl.pas                                                     }
{    Abstract:                                                            }
{        TreeServer User Manager user logon hours window.                 }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{                                                                         }
{    Revision                                                             }
{        Niu Jingyu    1998.07.16                                         }
{            Arrange the form.                                            }
{*************************************************************************}

unit clockctl;

interface
{$A-}

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ComCtrls, Grids, ExtCtrls, ToolWin, Buttons, StdCtrls, Math;

type
  TFrmClock = class(TForm)
    StGrdTimeCtl: TStringGrid;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    Label7: TLabel;
    BtnSure: TButton;
    BtnCancel: TButton;
    BtnAllow: TButton;
    Btnforbid: TButton;
    lbAllowTime: TLabel;
    lbForbitTime: TLabel;
    shpAllowTime: TShape;
    shpForbitTime: TShape;
    HeaderControl1: THeaderControl;
    Bevel1: TBevel;
    Bevel2: TBevel;
    Bevel3: TBevel;
    Image1: TImage;
    Image2: TImage;
    Image3: TImage;
    Bevel5: TBevel;
    Bevel6: TBevel;
    Bevel7: TBevel;
    Bevel4: TBevel;
    Bevel8: TBevel;
    Bevel9: TBevel;
    Bevel10: TBevel;
    Bevel11: TBevel;
    Bevel12: TBevel;
    Bevel13: TBevel;
    Label8: TLabel;
    Label9: TLabel;
    Label10: TLabel;
    Label11: TLabel;
    Label12: TLabel;
    procedure BtnCancelClick(Sender: TObject);
    procedure BtnSureClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure StGrdTimeCtlMouseUp(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure BtnAllowClick(Sender: TObject);
    procedure BtnforbidClick(Sender: TObject);
    procedure StGrdTimeCtlMouseMove(Sender: TObject; Shift: TShiftState; X,
      Y: Integer);
    procedure FormMouseMove(Sender: TObject; Shift: TShiftState; X,
      Y: Integer);
    procedure StGrdTimeCtlMouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

Procedure ReDrawCells;

var
  FrmClock: TFrmClock;

implementation

uses Main, PipeStruct, AcuntCtl;

var
  iPRect, iFRect : TRectInfo;
  TCRect : TGridRect;
  TPRect : TRect;
  GdS, Sel : Boolean;
  ComTimes : Array[0..MAX_LOGON_HOURS - 1] of Char;

//  isShow : Boolean;
{$R *.DFM}
procedure TFrmClock.BtnCancelClick(Sender: TObject);
begin
     FrmClock.Close;
end;

procedure TFrmClock.BtnSureClick(Sender: TObject);
begin
     CopyMemory ( @ComUserInfo.bLogonHours[0], @ComTimes[0], MAX_LOGON_HOURS );
     FrmClock.Close;
end;

procedure TFrmClock.FormShow(Sender: TObject);
var
   PTimes : Pointer;
begin
     ZeroMemory ( @iPRect, SizeOf ( TRectInfo ) );
     ZeroMemory ( @iFRect, SizeOf ( TRectInfo ) );
     CopyMemory ( @ComTimes[0], @ComUserInfo.bLogonHours[0], MAX_LOGON_HOURS );
     pTimes := @ComTimes[0];

     TransBinToGrid ( pTimes, iPRect, FrmClock.StGrdTimeCtl );
     Sel := False;
     //PostMessage( FrmClock.Handle, WM_MOUSEMOVE, 0, 0 );
     FrmClock.Repaint;
end;

procedure TFrmClock.StGrdTimeCtlMouseUp(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
var
   iTop, iLeft, iBottom, iRight : integer;
begin
     GdS := True;
     with stGrdTimeCtl, stGrdTimeCtl.Canvas do
     begin
          iTop := Selection.Top;
          iLeft := Selection.Left;
          iBottom := Selection.Bottom;
          iRight := Selection.Right;

          TCRect := Selection;

          with TPRect do
          begin
               Top := CellRect ( iLeft, iTop ).Top;
               Left := CellRect ( iLeft, iTop ).Left;
               Bottom := CellRect ( iRight, iBottom ).Bottom;
               Right := CellRect ( iRight, iBottom ).Right;
          end;

          ReDrawCells;
          Brush.Color := RGB ( 255,255, 0 );
          FillRect ( TPRect );
     end;
     Sel := True;
end;

procedure TFrmClock.BtnAllowClick(Sender: TObject);
var
   pTimes : Pointer;
begin
     if GdS then
     begin
          pTimes := @ComTimes[0];
          iPRect.iNum := iPRect.iNum + 1;
          iPRect.CRect[iPRect.iNum] := TCRect;
          iPRect.PRect[iPRect.iNum] := TPRect;
          if not TransGridPToBin ( pTimes, iPRect ) then exit;
          TransBinToGrid ( pTimes, iPRect, FrmClock.StGrdTimeCtl );
          StGrdTimeCtl.Repaint;
          GdS := False;
     end;
     Sel := False;
     ReDrawCells;
end;

procedure TFrmClock.BtnforbidClick(Sender: TObject);
var
   pTimes : Pointer;
begin
     if GdS then
     begin
          pTimes := @ComTimes[0];
          iFRect.iNum := iFRect.iNum + 1;
          iFRect.CRect[iFRect.iNum] := TCRect;
          iFRect.PRect[iFRect.iNum] := TPRect;
          if not TransGridFToBin ( pTimes, iFRect ) then exit;
          TransBinToGrid ( pTimes, iPRect, FrmClock.StGrdTimeCtl );
          StGrdTimeCtl.Repaint;
          Gds := False;
     end;
     Sel := False;
     ReDrawCells;
end;

Procedure ReDrawCells;
var
   i : integer;
begin
        for i := 1 to iPRect.iNum do
        begin
             FrmClock.StGrdTimeCtl.Canvas.Brush.Color := RGB ( 0,0, 255);
             FrmClock.StGrdTimeCtl.Canvas.FillRect ( iPRect.PRect[i] ) ;
        end;
end;

procedure TFrmClock.StGrdTimeCtlMouseMove(Sender: TObject;
  Shift: TShiftState; X, Y: Integer);
begin
     if not Sel then
     begin
          ReDrawCells;
     end;
end;

procedure TFrmClock.FormMouseMove(Sender: TObject; Shift: TShiftState; X,
  Y: Integer);
begin
     if not Sel then ReDrawCells;
end;

procedure TFrmClock.StGrdTimeCtlMouseDown(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
     Sel := False;
end;













end.
