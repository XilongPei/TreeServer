{*************************************************************************}
{    Module Name:                                                         }
{        Main.pas                                                         }
{    Abstract:                                                            }
{        TreeServer User logon computers edit window.                     }
{    Author:                                                              }
{        Wang Chao                                                        }
{    Copyright (c) ? 1998  China Railway Software Corporation.            }
{                                                                         }
{    Revision                                                             }
{        Niu Jingyu    1998.07.16                                         }
{            Format the error message.                                    }
{            Rewrite the code that the computer delete process logic.     }
{*************************************************************************}

unit logonCtl;

interface
{$A-}

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ComCtrls;

type
  TFrmLogCtl = class(TForm)
    GrpboxLogCtl: TGroupBox;
    lstBoxLogOn: TListBox;
    BtnSure: TButton;
    BtnCancel: TButton;
    procedure lstBoxLogOnDblClick(Sender: TObject);
    procedure BtnSureClick(Sender: TObject);
    procedure BtnCancelClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  FrmLogCtl: TFrmLogCtl;

implementation

uses PipeStruct, Main;
{$R *.DFM}

procedure TFrmLogCtl.lstBoxLogOnDblClick(Sender: TObject);
var
   InputString : String;
   DelIndex, i : Integer;
begin
     with lstBoxLogOn do
     begin
          if ItemIndex = Items.Count - 1 then
          begin
               InputString := InputBox ( '请输入工作站名称', '', '' );
               if InputString = '' then
                    exit;

               for i := 0 to Items.Count - 1 do
               begin
                    if InputString = Items[i] then
                    begin
                         MessageBox ( 0, '工作站已存在于登录列表中！', 'TreeServer 用户管理器', MB_Ok or MB_ICONINFORMATION );
                         exit;
                    end;
               end;
               Items.Insert( Items.Count - 1, InputString );
          end
          else
          begin
               { Rewrite by Niu Jingyu, 1998.07.16 }
               DelIndex := ItemIndex;
               if MessageBox( 0, PChar('从列表中删除工作站 '+Items.strings[DelIndex]+ '？'), 'TreeServer 用户管理器',
                              MB_OKCANCEL or MB_ICONQUESTION ) = 2 then
                   exit;
               Items.Delete( ItemIndex );
               ItemIndex := DelIndex;
               { End of rewrite. }
          end;
     end;
end;

procedure TFrmLogCtl.BtnSureClick(Sender: TObject);
var
   i : Integer;
   Temp : Array[0..MAX_LOGON_COMPUTER] of Char;
   Offset : integer;
begin
     with ComUserInfo, FrmLogCtl do
     begin
          ZeroMemory ( @SzLogonComputer[0], MAX_LOGON_COMPUTER);
          Offset := 0;

          for i := 0 to lstBoxLogOn.Items.Count - 2 do
          begin
               StrPCopy ( Temp, lstBoxLogOn.Items[i] );
               CopyMemory ( @SzLogonComputer[Offset], @Temp[0], Strlen( Temp ) + 1 );
               Offset := Offset + integer( StrLen( Temp ) ) + 1;
          end;

          SzLogonComputer[Offset] := #0;
          FrmLogCtl.Close;
     end;
end;

procedure TFrmLogCtl.BtnCancelClick(Sender: TObject);
begin
     FrmLogCtl.Close;
end;

procedure TFrmLogCtl.FormShow(Sender: TObject);
var
   IsEnd : Boolean;
   LogonComputer : Array[0..MAX_LOGON_COMPUTER] of Char;
   ALogonComputers : Array[0..19] of Array[0..MAX_COMPUTER] of Char;
   Len : integer;
   Max, i : integer;
begin
     Max := 0;
     IsEnd := False;

     lstBoxLogOn.Clear;
     lstBoxLogOn.Items.Add ( '    <新增工作站>');

     CopyMemory ( @LogonComputer, @ComUserInfo.szLogonComputer, MAX_LOGON_COMPUTER + 1 );

     if LogonComputer = '' then
        exit;
     FillMemory ( @ALogonComputers[0,0], MAX_LOGON_COMPUTER, 0 );

     while not IsEnd do
     begin
          StrCopy( AlogonComputers[Max], LogonComputer );
          Len := Strlen ( LogonComputer ) + 1;
          if LogonComputer[Len] <> #0 then
          begin
               IsEnd := False;
               Max := Max + 1;
               CopyMemory ( @LogonComputer[0],
                            @LogonComputer[Len],
                            MAX_LOGON_COMPUTER + 1 - Len );
          end
          else
              IsEnd := True;
     end;

     for i := 0 to Max do
     begin
          lstBoxLogOn.Items.Insert( lstBoxLogOn.Items.Count - 1, ALogonComputers[i] );
     end;
end;

end.
