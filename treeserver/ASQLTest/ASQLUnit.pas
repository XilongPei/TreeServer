unit ASQLUnit;

interface
uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ComCtrls, ExtCtrls, StdCtrls, inifiles,
  MainForms,Math;

type
  pXexpVar = ^XexpVar;
  XexpVar = record
    nType:smallint;
    VarOFunName:array [0..31] of char;
    Values:array [0..31] of char;
    Length:smallint;
  end;

var
  Run_ASQL_Local_Mode,Run_ASQL_TCPIP_Mode:BOOL;
  Asql_Mem:pchar;
  TreeSVR_handler:DWORD;
  MainServerName,Mainuser,Mainpassword,MainTCPIP:string;
  MainPath:string;
  Fhb_Mode:BOOL;

  function Run_Asql:BOOL;
  function Run_Asql_Var(mXexpVar:pXexpVar;mXexpVarNum:DWORD):BOOL;
  function Logon_Asql(user,password:string;const TCPIPport:string=''):boolean;
  procedure Logoff_Asql;
  function Change_ASQL_Password(oldPass,newPass:pchar):boolean;
  procedure Set_Work_Path;

  function Ret_RunTime(rtime:DWORD):string;

  function Local_callAsqlSvrM(szMem, szFromPath, szToPath:LPSTR):LPSTR;
                cdecl;far;external'AsqlClt.dll' name 'callAsqlSvrM';

  function Local_callAsqlSvrMVar(szMem, szFromPath, szToPath:LPSTR;lpXexpVar:pXexpVar;dwXexpVarNum:DWORD):LPSTR;
                cdecl;far;external'AsqlClt.dll' name 'callAsqlSvrMVar';

  function Ts_Logon(server,user,password:pchar; handler:lpdword): dword; cdecl;
      far;external 'tscapi.DLL' name 'tsLogon';

  function Ts_Logoff(handler:dword): boolean; cdecl;
      far;external 'tscapi.DLL' name 'tsLogoff';

  function tsCallAsqlSvrM( hConnect: THANDLE; szAsqkScripts: PCHAR; lpResult: PCHAR;
      lpdwLen: LPDWORD; szInstResult: PCHAR; dwInstResultSize: DWORD ): DWORD;
      cdecl;far;external 'tscapi.DLL' name 'tsCallAsqlSvrM';

  function tsGetUserNetPath(handler:dword;Path:pchar;Len:integer): DWORD; cdecl;
      far;external 'tscapi.DLL' name 'tsGetUserNetPath';

  function tsCallAsqlSvrMVar( hConnect: THANDLE; szAsqkScripts: PCHAR;
      lpXexpVar:pXexpVar;dwXexpVarNum:DWORD;
      szInstResult: PCHAR; dwInstResultSize: DWORD;
      lpResult: PCHAR;
      lpdwLen: LPDWORD): DWORD;
      cdecl;far;external 'tscapi.DLL' name 'tsCallAsqlSvrMVar';

  function tsGetIdenticalUserNum(handler:dword;user:pchar): DWORD; cdecl;
      far;external 'tscapi.DLL' name 'tsGetIdenticalUserNum';

  function tsUserChangePwd(handler:dword;uold,new:pchar): DWORD; cdecl;
      far;external 'tscapi.DLL' name 'tsUserChangePwd';

  function Ts_SetTcpipPort(szPortOrService:Pchar): dword; cdecl;
                    far;external 'tscapi.DLL' name 'tsSetTcpipPort';

implementation

function Ret_RunTime(rtime:DWORD):string;
  function Get_Run_Time(rtime:DWORD):string;
  var
    temp:dword;
  begin
    if rtime=0 then
    begin
      result:='';
      exit;
    end;
    if rtime<1000 then
    begin
      result:=inttostr(rtime)+' 毫秒';
      exit;
    end;
    if rtime<60000 then
    begin
      temp:=rtime div 1000;
      result:=inttostr(temp)+' 秒 '+get_run_time(rtime-(temp*1000) );
      exit;
    end;
    if rtime<60000*60 then
    begin
      temp:=rtime div 60000;
      result:=inttostr( temp )+' 分 '+get_run_time(rtime-temp*60000);
      exit;
    end;
    if rtime<60000*60*60 then
    begin
      temp:=rtime div (60000*60);
      result:=inttostr( temp )+' 时 '+get_run_time(rtime-temp*(60000*60));
      exit;
    end;
    if rtime>=60000*60*60 then
    begin
      result:='时间太长了！';
    end;
end;
begin
  result:=Get_Run_Time(rtime);
  if trim(result)='' then
    result:='0 秒';
end;

function Logon_Asql(user,password:string;const TCPIPport:string=''):boolean;
var
  i,ret:DWORD;
begin
  Ts_SetTcpipPort(pchar(TCPIPport));

  Logon_Asql:=false;
  screen.cursor:=crHourGlass;
  ret:=Ts_Logon(pchar(MainServerName),pchar(Mainuser),pchar(Mainpassword),@TreeSVR_handler);
  screen.cursor:=crDefault;
  if ret = $FFFFFFFF then
  begin
    Application.MessageBox(pchar('数据库服务器登录失败'), '提示框',
      MB_SYSTEMMODAL+MB_ok+MB_ICONEXCLAMATION);
    exit;
  end;
  i:=tsGetIdenticalUserNum(TreeSVR_handler,pchar(user));
  if i<>1 then
  begin
    Application.MessageBox(pchar('数据库服务器登录失败, 相同用户已存在'), '提示框',
      MB_SYSTEMMODAL+MB_ok+MB_ICONEXCLAMATION);
    exit;
  end;
  Logon_Asql:=true;
end;

procedure Logoff_Asql();
begin
  if not Ts_Logoff(TreeSVR_handler) then
    MessageDlg('TreeSVR 退出失败',mtinformation,[mbok],0);
end;

function Change_ASQL_Password(oldPass,newPass:pchar):boolean;
begin
  if tsUserChangePwd(TreeSVR_handler,oldPass,newPass)<>0 then
    Change_ASQL_Password:=false
  else
    Change_ASQL_Password:=true;
end;

function Run_Asql_Var(mXexpVar:pXexpVar;mXexpVarNum:DWORD):BOOL;
var
  ErrorStr1:pchar;
  ErrorStr,RsutStr:array [0..500] of char;
  ErrorLen,RsutLen:DWORD;
  runtime:dword;
begin
  if Run_ASQL_Local_Mode then
  begin
    screen.cursor:=crHourGlass;
    runtime:=GetTickCount;
    ErrorStr1:=Local_callAsqlSvrMVar(Asql_Mem, nil, nil,mXexpVar,mXexpVarNum);
    mainform.StatusBar.Panels[1].Text:='执行时间 '+Ret_RunTime(GetTickCount-runtime);
    screen.cursor:=crDefault;
    if ErrorStr1<>'ERR:0' then
    begin
      if trim(ErrorStr1)='' then
        Application.MessageBox('和数据库服务器的联系中断', '提示框',
          MB_SYSTEMMODAL+mb_ok+MB_ICONstop);
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clRed;
      MainForm.MsgEditor.SetTextBuf(ErrorStr1);
      Run_Asql_Var:=false;
    end
    else
    begin
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clGreen;
      MainForm.MsgEditor.SetTextBuf(ErrorStr1);
      Run_Asql_Var:=true;
    end;
    exit;
  end
  else
  begin
    ErrorLen:=500;
    RsutLen:=500;
    ZeroMemory(@ErrorStr[0], 500 );
    ZeroMemory(@RsutStr[0], 500 );

    screen.cursor:=crHourGlass;
    runtime:=GetTickCount;
    tsCallAsqlSvrMVar(TreeSVR_handler,Asql_Mem,mXexpVar,mXexpVarNum,
         @ErrorStr,ErrorLen,@RsutStr[0],@RsutLen);
    mainform.StatusBar.Panels[1].Text:='执行时间 '+Ret_RunTime(GetTickCount-runtime);
    screen.cursor:=crDefault;
    if ErrorStr<>'ERR:0' then
    begin
      if trim(ErrorStr)='' then
        Application.MessageBox('和数据库服务器的联系中断', '提示框',
          MB_SYSTEMMODAL+mb_ok+MB_ICONstop);
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clRed;
      MainForm.MsgEditor.SetTextBuf(ErrorStr);
      Run_Asql_Var:=false;
    end
    else
    begin
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clGreen;
      MainForm.MsgEditor.SetTextBuf(ErrorStr);
      Run_Asql_Var:=true;
    end;
  end;
end;

function Run_Asql:BOOL;
var
  ErrorStr1:pchar;
  ErrorStr,RsutStr:array [0..500] of char;
  ErrorLen,RsutLen:DWORD;
  runtime:dword;
begin
  if Run_ASQL_Local_Mode then
  begin
    screen.cursor:=crHourGlass;
    runtime:=GetTickCount;
    ErrorStr1:=Local_callAsqlSvrM(Asql_Mem, nil, nil);
    mainform.StatusBar.Panels[1].Text:='执行时间 '+Ret_RunTime(GetTickCount-runtime);
    screen.cursor:=crDefault;
    if ErrorStr1<>'ERR:0' then
    begin
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clRed;
      MainForm.MsgEditor.SetTextBuf(ErrorStr1);
      Run_Asql:=false;
    end
    else
    begin
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clGreen;
      MainForm.MsgEditor.SetTextBuf(ErrorStr1);
      Run_Asql:=true;
    end;
    exit;
  end
  else
  begin
    ErrorLen:=500;
    RsutLen:=500;
    ZeroMemory(@ErrorStr[0], 500 );
    ZeroMemory(@RsutStr[0], 500 );

    screen.cursor:=crHourGlass;
    runtime:=GetTickCount;
    tsCallAsqlSvrM(TreeSVR_handler,Asql_Mem,@RsutStr[0],@RsutLen,@ErrorStr,ErrorLen);
    mainform.StatusBar.Panels[1].Text:='执行时间 '+Ret_RunTime(GetTickCount-runtime);
    screen.cursor:=crDefault;
    if ErrorStr<>'ERR:0' then
    begin
      if trim(ErrorStr)='' then
        Application.MessageBox('和数据库服务器的联系中断', '提示框',
          MB_SYSTEMMODAL+mb_ok+MB_ICONstop);
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clRed;
      MainForm.MsgEditor.SetTextBuf(ErrorStr);
      Run_Asql:=false;
    end
    else
    begin
      MainForm.MsgEditor.Lines.Clear;
      MainForm.MsgEditor.Font.Color:=clGreen;
      MainForm.MsgEditor.SetTextBuf(ErrorStr);
      Run_Asql:=true;
    end;
  end;
end;

procedure Set_Work_Path;
{$IFDEF Local_Mod}
var
  IniFile:TiniFile;
begin
  IniFile:=TiniFile.create(MainIni);
  SetCurrentDir(IniFile.ReadString('Configuration','Work Path',GetCurrentDir()));
end;
{$ELSE}
var
  Path:array[0..100] of char;
begin
  ZeroMemory(@Path[0], 100 );
  tsGetUserNetPath (TreeSVR_Handler,@Path[0],100);
  SetCurrentDir(Path);
end;
{$ENDIF}

end.
