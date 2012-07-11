#ifndef _INC_NAMEDPIPE_
#define _INC_NAMEDPIPE_

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for application 
 * default to 1 byte alignment.
 */
#pragma pack(push,1)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef _NAMEDPIPE_MAIN_

__declspec( dllexport )
HANDLE CliOpenTSNamedPipe (
	LPCSTR lpServer );

__declspec( dllexport )
BOOL CliCloseTSNamedPipe ( 
	HANDLE hPipe );

__declspec( dllexport )
DWORD CliTSLogon ( 
	HANDLE hPipe,
	LPCSTR lpszUser,
	LPCSTR lpszPasswd, 
	LPCSTR lpszComputer );

__declspec( dllexport )
BOOL CliTSLogoff ( 
	HANDLE hPipe );

__declspec( dllexport )
DWORD CliReadTSNamedPipe ( 
	HANDLE hPipe,
	LPVOID lpBuffer,
	LPDWORD lpdwcbBuffer );

__declspec( dllexport )
DWORD CliWriteTSNamedPipe ( 
	HANDLE hPipe,
	LPVOID lpBuffer,
	LPDWORD lpdwcbBuffer );

__declspec( dllexport )
DWORD justRunASQL (
	HANDLE hPipe,
	LPSTR lpParam,
	LPSTR lpResponse,
	DWORD dwParam );

__declspec( dllexport )
DWORD CliTSPipeFTPPut ( 
	HANDLE hPipe, 
	LPSTR lpLocalFile, 
	LPSTR lpRemoteFile );

__declspec( dllexport )
DWORD CliTSPipeFTPGet ( 
	HANDLE hPipe, 
	LPSTR lpRemoteFile, 
	LPSTR lpLocalFile );

__declspec( dllexport )
DWORD RunASQL ( HANDLE hPipe, 
	LPSTR lpParam, 
	LPSTR szGetFiles, 
	LPSTR szLocalPath, 
	LPSTR lpResponse, 
	DWORD dwParam );

__declspec( dllexport )
int cliSetTcpipPort( char *szPortOrService );

#else

__declspec( dllimport )
HANDLE CliOpenTSNamedPipe (
	LPCSTR lpServer );

__declspec( dllimport )
BOOL CliCloseTSNamedPipe ( 
	HANDLE hPipe );

__declspec( dllimport )
DWORD CliTSLogon ( 
	HANDLE hPipe,
	LPCSTR lpszUser,
	LPCSTR lpszPasswd, 
	LPCSTR lpszComputer );

__declspec( dllimport )
BOOL CliTSLogoff ( 
	HANDLE hPipe );

__declspec( dllimport )
DWORD CliReadTSNamedPipe ( 
	HANDLE hPipe,
	LPVOID lpBuffer,
	LPDWORD lpdwcbBuffer );

__declspec( dllimport )
DWORD CliWriteTSNamedPipe ( 
	HANDLE hPipe,
	LPVOID lpBuffer,
	LPDWORD lpdwcbBuffer );

__declspec( dllimport )
DWORD justRunASQL (
	HANDLE hPipe,
	LPSTR lpParam,
	LPSTR lpResponse,
	DWORD dwParam );

__declspec( dllimport )
DWORD CliTSPipeFTPPut ( 
	HANDLE hPipe,
	LPSTR lpLocalFile, 
	LPSTR lpRemoteFile );

__declspec( dllimport )
DWORD CliTSPipeFTPGet ( 
	HANDLE hPipe, 
	LPSTR lpRemoteFile, 
	LPSTR lpLocalFile );

__declspec( dllimport )
DWORD RunASQL ( HANDLE hPipe, 
	LPSTR lpParam, 
	LPSTR szGetFiles, 
	LPSTR szLocalPath, 
	LPSTR lpResponse, 
	DWORD dwParam );

__declspec( dllimport )
int cliSetTcpipPort( char *szPortOrService );

#endif // _NAMEDPIPE_MAIN_

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif // _INC_NAMEDPIPE_