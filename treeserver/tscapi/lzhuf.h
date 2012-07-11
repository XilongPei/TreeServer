# if !defined( MODULEEXE ) 
# define MODULEAPI __declspec( dllexport )
# else
# define MODULEAPI __declspec( dllimport )
# endif

#ifdef __cplusplus
extern "C" {
#endif

MODULEAPI int MemCompressIntoMem( char *dst, long *dstLen, char *src, long srcLen );
MODULEAPI int MemDecompressIntoMem( char *dst, long *dstLen, char *src, long srcLen );
MODULEAPI int FileCompressIntoFile( char *szinFileName, char *szoutFileName );
MODULEAPI int FileDecompressIntoFile( char *szinFileName, char *szoutFileName );

int FileCompressToSend(LPSTR lpFileName, void *lpExchBufInfo);
int FileDecompressFromGet(LPSTR lpFileName, void *lpExchBufInfo);

#ifdef __cplusplus
}
#endif

