unit DesApi;

interface

uses windows;

type LPBYTE = ^Byte;

procedure DES( key : LPCSTR; text : LPCSTR; mtext : LPBYTE ); cdecl; external 'TSCAPI.DLL' name 'DES';
procedure _DES( key : LPCSTR; text : LPCSTR; mtext : LPBYTE ); cdecl; external 'TSCAPI.DLL' name '_DES';

implementation

end.
