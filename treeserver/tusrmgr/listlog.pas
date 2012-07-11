unit listlog;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs, pipefunc,
  StdCtrls, Grids, TS_COM, PipeStruct, terror, ExtCtrls;

type
  TForm1 = class(TForm)
    StringGrid1: TStringGrid;
    Button1: TButton;
    Button2: TButton;
    Timer2: TTimer;
    procedure FormActivate(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure Timer2Timer(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

  ClinetResInfo = record
		szUser: array [1..21] of char;
		szComputer: array [1..16] of char;
		hToken:THANDLE;
		tLogonTime:TSYSTEMTIME;
		dwAgentThreadId:DWORD;
		dwServiceThreadId: DWORD;
  end;

  LOGONINFO = array [1..10] of ClinetResInfo;

  PClinetResInfo = ^LOGONINFO;

var
  Form1: TForm1;

  function ts_GetLog(handle:THANDLE; cliet:PClinetResInfo; pnum: LPDWORD):longint;
  		cdecl;far;external 'TSCAPI.DLL' name 'tsGetRegisteredUser';

implementation

uses main;

{$R *.DFM}

procedure TForm1.FormActivate(Sender: TObject);
var
   Buffer : array[0 .. 4095] of Char;
//   lpTsComProps : PTTS_COM_PROPS;
//   dwResult : DWORD;
//   IsEnd: Boolean;
   dwcbBuffer : DWORD;
//   lpLogonInfo : ClinetResInfo;
   i           : integer;
   PClinet     : PClinetResInfo;
begin

   TUserMgr.Timer1.Enabled:=False;
   Timer2.Enabled:=True;

   dwcbBuffer := PIPE_BUFFER_SIZE;
   PClinet := PClinetResInfo( @Buffer[0] );
   ts_GetLog(ClihPipe, PClinet, @dwcbBuffer);

   //dwcbBuffer, 用户数
   StringGrid1.rowcount:=dwcbBuffer+1;

   //list every user information
   StringGrid1.Cells[0,0] := '序号';
   StringGrid1.ColWidths[0]:=40;
   StringGrid1.Cells[1,0] := '用户名';
   StringGrid1.ColWidths[1]:=130;
   StringGrid1.Cells[2,0] := '机器名';
   StringGrid1.ColWidths[2]:=110;
   StringGrid1.Cells[3,0] := '登录时间';
   StringGrid1.ColWidths[3]:=200;

   //dwcbBuffer
   for i:=1 to dwcbBuffer do
   begin
        if( PClinet[i].szUser <> '' ) then
        begin
   	     StringGrid1.Cells[0,i] := inttostr(i);
   	     StringGrid1.Cells[1,i] := pchar(string(PClinet[i].szUser));
             StringGrid1.Cells[2,i] := pchar(string(PClinet[i].szComputer));
             StringGrid1.Cells[3,i] := inttostr(PClinet[i].tLogonTime.wYear)+'年'+
                                       inttostr(PClinet[i].tLogonTime.wMonth)+'月'+
                                       inttostr(PClinet[i].tLogonTime.wDay)+'日 星期'+
                                       inttostr(PClinet[i].tLogonTime.wDayOfWeek)+'  '+
                                       inttostr(PClinet[i].tLogonTime.wHour)+'时'+
                                       inttostr(PClinet[i].tLogonTime.wMinute)+'分'+
                                       inttostr(PClinet[i].tLogonTime.wSecond)+'秒';
        end;
   end;

end;

procedure TForm1.Button1Click(Sender: TObject);
begin
    close;
end;

procedure TForm1.Button2Click(Sender: TObject);
begin
     FormActivate(Sender);
end;

procedure TForm1.FormClose(Sender: TObject; var Action: TCloseAction);
begin
    TUserMgr.Timer1.Enabled:=True;
    Timer2.Enabled:=False;
end;

procedure TForm1.Timer2Timer(Sender: TObject);
begin
    FormActivate(Sender);
end;

end.
