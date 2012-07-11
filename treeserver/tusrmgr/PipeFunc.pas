unit PipeFunc;

interface
{$A-}

uses windows, Dialogs, SysUtils, PipeStruct;

{Base TSNamedPipe Operation}

{$IFDEF OldVersion}
function CliOpenTSNamedPipe(Server : PChar) : DWORD;
function CliCloseTSNamedPipe (hPipe : DWORD) : Boolean;
function CliTSLogon (hPipe : DWORD; lpszUser : PChar; lpszPasswd : PChar;
					lpszComputer : PChar) : DWORD;
function CliTSLogoff (hPipe : DWORD) : Boolean;
function CliReadTSNamedPipe (hPipe : DWORD; lpBuffer : PChar;
                                   lpdwcbBuffer :LPDWORD) : DWORD;
function CliWriteTSNamedPipe (hPipe : DWORD; lpBuffer : PChar;
                                      lpdwcbBuffer :LPDWORD) : DWORD;
{$ELSE}
function CliOpenTSNamedPipe(Server : PChar) : DWORD;
         cdecl; external 'tscapi.dll' name 'CliOpenTSNamedPipe';
function CliCloseTSNamedPipe(Server : DWORD) : Boolean;
         cdecl; external 'tscapi.dll' name 'CliCloseTSNamedPipe';
function CliTSLogon (hPipe : DWORD; lpszUser : PChar; lpszPasswd : PChar;
					lpszComputer : PChar) : DWORD;
         cdecl; external 'tscapi.dll' name 'CliTSLogon';
function CliTSLogoff (hPipe : DWORD) : Boolean;
         cdecl; external 'Tscapi.dll' name 'tsLogoff';
         //'CliTSLogoff';
function CliReadTSNamedPipe (hPipe : DWORD; lpBuffer : PChar;
                                   lpdwcbBuffer :LPDWORD) : DWORD;
         cdecl; external 'tscapi.dll' name 'CliReadTSNamedPipe';
function CliWriteTSNamedPipe (hPipe : DWORD; lpBuffer : PChar;
                                      lpdwcbBuffer :LPDWORD) : DWORD;
         cdecl; external 'tscapi.dll' name 'CliWriteTSNamedPipe';
{$ENDIF}

implementation

{$IFDEF OldVersion}
uses ts_com, terror;

function CliOpenTSNamedPipe(Server : PChar) : DWORD;
var
   szPipeName : array[0..MAX_PATH - 1] of Char;
   hPipe : DWORD;
   fSuccess : Boolean;
   iTryCnt: Integer;
begin
     if Server <> nil then
     begin
          StrCopy(szPipeName, '\\');
          StrCat(szPipeName, Server);
          StrCat(szPipeName, '\pipe\TS_ConvPipe');
     end
     else
     begin
          SetLastError( ERROR_INVALID_PARAMETER );
          Result := INVALID_HANDLE_VALUE;
          exit;
     end;

     fSuccess := False;
     iTryCnt := 0;
     while (not fSuccess) and (iTryCnt < 5) do begin
         Inc( iTryCnt );
         fSuccess := WaitNamedPipe(szPipeName, 1000);
     end;

     if not fSuccess then
     begin
          Result := INVALID_HANDLE_VALUE;
          exit;
     end;

     hPipe := CreateFile ( szPipeName,
                           GENERIC_READ or GENERIC_WRITE,
                           0,
                           nil,
                           OPEN_EXISTING,
                           0,
                           0 );

     if ( hPipe = INVALID_HANDLE_VALUE ) then
     begin
          Result := INVALID_HANDLE_VALUE;
          exit;
     end;

     SetLastError( TERR_SUCCESS );
     Result := hPipe;
end;

function CliCloseTSNamedPipe (hPipe : DWORD) : Boolean;
begin
     if hPipe = INVALID_HANDLE_VALUE then
     begin
          SetLastError( ERROR_INVALID_PARAMETER );
          Result := FALSE;
          exit;
     end;
     Result := CloseHandle( hPipe );
end;

function CliTSLogon (hPipe : DWORD; lpszUser : PChar; lpszPasswd : PChar;
					lpszComputer : PChar) : DWORD;
var
   Buffer : array[0..PIPE_BUFFER_SIZE - 1] of char;
   TempStr : array[0..255] of Char;
   lptsComProps : PTTS_COM_PROPS;
   dwcbBuffer : Longint;
   dwRetCode : DWORD;
begin
     if (hPipe = INVALID_HANDLE_VALUE) or (lpszUser = nil)
        or (lpszPasswd = nil) or (lpszComputer = nil) then
     begin
          SetLastError( ERROR_INVALID_PARAMETER );
          Result := $FFFFFFFF;
          exit;
     end;
     ZeroMemory(@Buffer[0], PIPE_BUFFER_SIZE);
     lptsComProps := PTTS_COM_PROPS(@Buffer[0]);

     ZeroMemory ( @TempStr[0], 256 );
     StrCopy( TempStr, lpszUser );
//     StrCat ( TempStr, #10 );
     StrCopy( @TempStr[16], lpszPassWd );
//     StrCat ( TempStr, #10 );
     StrCopy( @TempStr[32], lpszComputer );

     CopyMemory ( Pointer ( LongInt ( @Buffer ) + SizeOf ( TTS_COM_PROPS ) ),
                  @TempStr[0],
                  {StrLen ( TempStr )}256 );

     with lptsComProps^ do
     begin
          packetType := pkTS_SYNC_PIPE;
	  msgType := msgTS_CLIENT_LOGON;
          len := {StrLen(TempStr)}256;
     end;

     dwcbBuffer := PIPE_BUFFER_SIZE;
     dwRetCode := CliWriteTSNamedPipe( hPipe, Buffer, @dwcbBuffer );
     if( dwRetCode <> TERR_SUCCESS ) then
     begin
          Result := $FFFFFFFF;
          exit;
     end;

     dwcbBuffer := PIPE_BUFFER_SIZE;
     dwRetCode := CliReadTSNamedPipe( hPipe, Buffer, @dwcbBuffer );
     if( dwRetCode <> TERR_SUCCESS ) then
     begin
          Result := $FFFFFFFF;
          exit;
     end;

     if( lptsComProps.msgType <> msgTS_LOGON_OK ) then
     begin
          SetLastError( lptsComProps.lp );
          Result := $FFFFFFFF;
          exit;
     end;

     Result := lptsComProps.lp;
end;

function CliTSLogoff (hPipe : DWORD) : Boolean;
var
   tsComProps : TTS_COM_PROPS;
   dwBufSize : DWORD;
   dwResult : DWORD;
   Buffer : Array[0..PIPE_BUFFER_SIZE - 1] of char;
begin
     if( hPipe = INVALID_HANDLE_VALUE ) then
     begin
          SetLastError( ERROR_INVALID_PARAMETER );
          Result := FALSE;
          exit;
     end;

     ZeroMemory( @tsComProps, sizeof(TTS_COM_PROPS) );
     tsComProps.packetType := pkTS_SYNC_PIPE;
     tsComProps.msgType := msgTS_PIPE_CLOSE;
     tsComProps.len := 0;

     dwBufSize := PIPE_BUFFER_SIZE;
     CopyMemory ( @Buffer[0], @tsComProps, SizeOf ( TTS_COM_PROPS ) );

     dwResult := CliWriteTSNamedPipe ( hPipe, Buffer, @dwBufSize );
     if( dwResult <> TERR_SUCCESS ) then
         Result := FALSE
     else
         Result := TRUE;
end;

function CliReadTSNamedPipe (hPipe : DWORD; lpBuffer : PChar;
                                   lpdwcbBuffer :LPDWORD) : DWORD;
var
   dwcbBuffer : DWORD;
   fSuccess : Boolean;
   Buffer : Array[0..PIPE_BUFFER_SIZE - 1] of char;
begin
     if (hPipe = INVALID_HANDLE_VALUE) or (lpBuffer = nil) or
               (lpdwcbBuffer = nil) or (lpdwcbBuffer^ > PIPE_BUFFER_SIZE) then
     begin
          SetLastError( ERROR_INVALID_PARAMETER );
          Result := ERROR_INVALID_PARAMETER;
          exit;
     end;

     dwcbBuffer := lpdwcbBuffer^;
     FillMemory ( @Buffer[0], PIPE_BUFFER_SIZE, 0 );

     fSuccess := ReadFile ( hPipe,
			   Buffer,
			   dwcbBuffer,
			   lpdwcbBuffer^,
			   nil );
    if ( not fSuccess ) or ( lpdwcbBuffer^ = 0 ) then
    begin
         Result := GetLastError();
         exit;
    end;

    FlushFileBuffers ( hPipe );
    CopyMemory ( lpBuffer, @Buffer[0], PIPE_BUFFER_SIZE );

    Result := TERR_SUCCESS;
end;

function CliWriteTSNamedPipe (hPipe : DWORD; lpBuffer : PChar;
                                      lpdwcbBuffer :LPDWORD) : DWORD;
var
   lptsComProps : PTTS_COM_PROPS;
   dwcbBuffer : {Longint}DWORD;
   fSuccess : Boolean;
   Buffer : array[0..PIPE_BUFFER_SIZE - 1] of char;
begin
     if (hPipe = INVALID_HANDLE_VALUE) or (lpBuffer = nil) or
               (lpdwcbBuffer = nil) or (lpdwcbBuffer^ > PIPE_BUFFER_SIZE) then
     begin
          SetLastError( ERROR_INVALID_PARAMETER );
          Result := ERROR_INVALID_PARAMETER;
          exit;
     end;

     lptsComProps := PTTS_COM_PROPS(lpBuffer);
     dwcbBuffer := lptsComProps.len + sizeof(TTS_COM_PROPS);
     CopyMemory ( @Buffer[0], lpBuffer, PIPE_BUFFER_SIZE );
     fSuccess := WriteFile ( hPipe,
			    Buffer,
			    dwcbBuffer,
			    lpdwcbBuffer^,
			    nil );

    if ( not fSuccess ) or ( lpdwcbBuffer^ <> dwcbBuffer ) then
    begin
         Result := GetLastError();
         Exit;
    end;

    FlushFileBuffers ( hPipe );

    Result := TERR_SUCCESS;
end;
{$ENDIF}

end.
