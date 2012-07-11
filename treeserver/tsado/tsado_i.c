/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Feb 13 10:12:48 2001
 */
/* Compiler settings for tsado.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID LIBID_TSADOLib = {0xCD238363,0x4F88,0x11D3,{0xBF,0xE1,0x00,0x00,0xE8,0xE7,0xCE,0x21}};


const CLSID CLSID_Command = {0xCD238373,0x4F88,0x11D3,{0xBF,0xE1,0x00,0x00,0xE8,0xE7,0xCE,0x21}};


const IID IID_ICommand = {0xCD238372,0x4F88,0x11D3,{0xBF,0xE1,0x00,0x00,0xE8,0xE7,0xCE,0x21}};


#ifdef __cplusplus
}
#endif

