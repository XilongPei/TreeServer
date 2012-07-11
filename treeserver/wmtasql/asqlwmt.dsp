# Microsoft Developer Studio Project File - Name="asqlwmt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=asqlwmt - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "asqlwmt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "asqlwmt.mak" CFG="asqlwmt - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "asqlwmt - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asqlwmt - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "asqlwmt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp1 /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NTSDK" /D "ASQL_NT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib netapi32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib"

!ELSEIF  "$(CFG)" == "asqlwmt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\asqlwmt_"
# PROP BASE Intermediate_Dir ".\asqlwmt_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp1 /MDd /W3 /Gm /GX /Zi /Od /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_NTSDK" /D "ASQL_NT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib netapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libc.lib"

!ENDIF 

# Begin Target

# Name "asqlwmt - Win32 Release"
# Name "asqlwmt - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\Arrayutl.c
# End Source File
# Begin Source File

SOURCE=.\ascii_1.c
# End Source File
# Begin Source File

SOURCE=.\ascii_2.c
# End Source File
# Begin Source File

SOURCE=.\ascii_3.c
# End Source File
# Begin Source File

SOURCE=.\Ascii_4.c
# End Source File
# Begin Source File

SOURCE=.\Ascii_5.c
# End Source File
# Begin Source File

SOURCE=.\Ascii_6.c
# End Source File
# Begin Source File

SOURCE=.\ascii_7.c
# End Source File
# Begin Source File

SOURCE=.\Askact.c
# End Source File
# Begin Source File

SOURCE=.\Askact2.c
# End Source File
# Begin Source File

SOURCE=.\Askactuv.c
# End Source File
# Begin Source File

SOURCE=.\Asql_opt.c
# End Source File
# Begin Source File

SOURCE=.\asql_xml.c
# End Source File
# Begin Source File

SOURCE=.\Asqlana.c
# End Source File
# Begin Source File

SOURCE=.\Asqlerr.c
# End Source File
# Begin Source File

SOURCE=.\Asqlmain.c
# End Source File
# Begin Source File

SOURCE=.\Asqlpath.c
# End Source File
# Begin Source File

SOURCE=.\Asqlutl.c
# End Source File
# Begin Source File

SOURCE=.\Bc2watc.c
# End Source File
# Begin Source File

SOURCE=.\Btree.c
# End Source File
# Begin Source File

SOURCE=.\Btreeadd.c
# End Source File
# Begin Source File

SOURCE=.\Btreeext.c
# End Source File
# Begin Source File

SOURCE=.\Btreewmt.c
# End Source File
# Begin Source File

SOURCE=.\Cfg_fast.c
# End Source File
# Begin Source File

SOURCE=.\charutl.c
# End Source File
# Begin Source File

SOURCE=.\Db_s_ndx.c
# End Source File
# Begin Source File

SOURCE=.\Dbs_util.c
# End Source File
# Begin Source File

SOURCE=.\des.c
# End Source File
# Begin Source File

SOURCE=.\Dio.c
# End Source File
# Begin Source File

SOURCE=.\Diodbt.c
# End Source File
# Begin Source File

SOURCE=.\Dioext.c
# End Source File
# Begin Source File

SOURCE=.\Diofile.c
# End Source File
# Begin Source File

SOURCE=.\ExchBuff.c
# End Source File
# Begin Source File

SOURCE=.\Filetool.c
# End Source File
# Begin Source File

SOURCE=.\Histdata.c
# End Source File
# Begin Source File

SOURCE=.\Hzth.c
# End Source File
# Begin Source File

SOURCE=.\Macro.c
# End Source File
# Begin Source File

SOURCE=.\Memutl.c
# End Source File
# Begin Source File

SOURCE=.\Mistools.c
# End Source File
# Begin Source File

SOURCE=.\Mistring.c
# End Source File
# Begin Source File

SOURCE=.\Mxccfg.c
# End Source File
# Begin Source File

SOURCE=.\Ndx_man.c
# End Source File
# Begin Source File

SOURCE=.\odbc_dio.c
# End Source File
# Begin Source File

SOURCE=.\Parabrk.c
# End Source File
# Begin Source File

SOURCE=.\Rjio.c
# End Source File
# Begin Source File

SOURCE=.\Str_gram.c
# End Source File
# Begin Source File

SOURCE=.\Strutl.c
# End Source File
# Begin Source File

SOURCE=.\Syscall.c
# End Source File
# Begin Source File

SOURCE=.\t_int64.c
# End Source File
# Begin Source File

SOURCE=.\T_lock_r.c
# End Source File
# Begin Source File

SOURCE=.\Tabtools.c
# End Source File
# Begin Source File

SOURCE=.\treepara.c
# End Source File
# Begin Source File

SOURCE=.\treevent.c
# End Source File
# Begin Source File

SOURCE=.\Ts_dict.c
# End Source File
# Begin Source File

SOURCE=.\Tsdbfutl.c
# End Source File
# Begin Source File

SOURCE=.\Wmtasql.c
# End Source File
# Begin Source File

SOURCE=.\wst2mt.c
# End Source File
# Begin Source File

SOURCE=.\Xexp.c
# End Source File
# Begin Source File

SOURCE=.\Xexpfun.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Arrayutl.h
# End Source File
# Begin Source File

SOURCE=.\Asqlana.h
# End Source File
# Begin Source File

SOURCE=.\Asqlerr.h
# End Source File
# Begin Source File

SOURCE=.\Asqlutl.h
# End Source File
# Begin Source File

SOURCE=.\Blob.h
# End Source File
# Begin Source File

SOURCE=.\Btree.h
# End Source File
# Begin Source File

SOURCE=.\Btreeadd.h
# End Source File
# Begin Source File

SOURCE=.\Btreeext.h
# End Source File
# Begin Source File

SOURCE=.\Cfg_fast.h
# End Source File
# Begin Source File

SOURCE=.\Datetime.h
# End Source File
# Begin Source File

SOURCE=.\Decimal.h
# End Source File
# Begin Source File

SOURCE=.\Dio.h
# End Source File
# Begin Source File

SOURCE=.\Diodbt.h
# End Source File
# Begin Source File

SOURCE=.\Dioext.h
# End Source File
# Begin Source File

SOURCE=.\dir.h
# End Source File
# Begin Source File

SOURCE=.\ExchBuff.h
# End Source File
# Begin Source File

SOURCE=.\filetool.h
# End Source File
# Begin Source File

SOURCE=.\Hzth.h
# End Source File
# Begin Source File

SOURCE=.\Idbase.h
# End Source File
# Begin Source File

SOURCE=.\Locator.h
# End Source File
# Begin Source File

SOURCE=.\Macro.h
# End Source File
# Begin Source File

SOURCE=.\Memutl.h
# End Source File
# Begin Source File

SOURCE=.\Mistools.h
# End Source File
# Begin Source File

SOURCE=.\Mistring.h
# End Source File
# Begin Source File

SOURCE=.\Odbc_dio.h
# End Source File
# Begin Source File

SOURCE=.\parabrk.h
# End Source File
# Begin Source File

SOURCE=.\Pubvar.h
# End Source File
# Begin Source File

SOURCE=.\Readcfg.h
# End Source File
# Begin Source File

SOURCE=.\Sqlca.h
# End Source File
# Begin Source File

SOURCE=.\Sqlda.h
# End Source File
# Begin Source File

SOURCE=.\Sqlhdr.h
# End Source File
# Begin Source File

SOURCE=.\Sqlproto.h
# End Source File
# Begin Source File

SOURCE=.\strutl.h
# End Source File
# Begin Source File

SOURCE=.\syscall.h
# End Source File
# Begin Source File

SOURCE=.\Tabtools.h
# End Source File
# Begin Source File

SOURCE=.\TAccount.h
# End Source File
# Begin Source File

SOURCE=.\terror.h
# End Source File
# Begin Source File

SOURCE=.\tlimits.h
# End Source File
# Begin Source File

SOURCE=.\treepara.h
# End Source File
# Begin Source File

SOURCE=.\Treevent.h
# End Source File
# Begin Source File

SOURCE=.\Ts_const.h
# End Source File
# Begin Source File

SOURCE=.\Ts_dict.h
# End Source File
# Begin Source File

SOURCE=.\TSCommon.h
# End Source File
# Begin Source File

SOURCE=.\Tsdbfutl.h
# End Source File
# Begin Source File

SOURCE=.\Util.h
# End Source File
# Begin Source File

SOURCE=.\Uvutl.h
# End Source File
# Begin Source File

SOURCE=.\Value.h
# End Source File
# Begin Source File

SOURCE=.\Wmtasql.h
# End Source File
# Begin Source File

SOURCE=.\Wst2mt.h
# End Source File
# Begin Source File

SOURCE=.\Xexp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Script1.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\btree\Debug\dbtree.lib
# End Source File
# Begin Source File

SOURCE=..\TAccount\Debug\taccount.lib
# End Source File
# Begin Source File

SOURCE=..\tscommon\Debug\tscommon.lib
# End Source File
# Begin Source File

SOURCE=.\Debug\sql_dio.lib
# End Source File
# End Target
# End Project
