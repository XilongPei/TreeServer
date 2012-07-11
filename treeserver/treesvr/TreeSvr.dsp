# Microsoft Developer Studio Project File - Name="TreeSvr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TreeSvr - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TreeSvr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TreeSvr.mak" CFG="TreeSvr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TreeSvr - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TreeSvr - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TreeSvr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp1 /MD /W3 /GX /O2 /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_MT" /D "_NTSDK" /D "TREESVR_STANDALONE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /fo"Debug/treesvr.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "TreeSvr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Zp1 /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MT" /D "_NTSDK" /D "TREESVR_STANDALONE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "TreeSvr - Win32 Release"
# Name "TreeSvr - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.c;*.cpp;*.def;*.rc"
# Begin Source File

SOURCE=..\src\Conv.c
# End Source File
# Begin Source File

SOURCE=..\src\des.c
# End Source File
# Begin Source File

SOURCE=..\src\ExchBuff.c
# End Source File
# Begin Source File

SOURCE=..\src\Notify.c
# End Source File
# Begin Source File

SOURCE=..\src\PASSWORD.C
# End Source File
# Begin Source File

SOURCE="..\src\plug-ins.c"
# End Source File
# Begin Source File

SOURCE=..\src\S_Proc.c
# End Source File
# Begin Source File

SOURCE=..\src\schedule.c
# End Source File
# Begin Source File

SOURCE=..\src\secapi.c
# End Source File
# Begin Source File

SOURCE=..\src\Shell.c
# End Source File
# Begin Source File

SOURCE=..\src\TEngine.c
# End Source File
# Begin Source File

SOURCE=..\src\terror.c
# End Source File
# Begin Source File

SOURCE=..\src\tqueue.c
# End Source File
# Begin Source File

SOURCE=..\src\Tree_tcp.c
# End Source File
# Begin Source File

SOURCE=..\src\treesvr.c
# End Source File
# Begin Source File

SOURCE=..\src\TreeSvr.rc
# End Source File
# Begin Source File

SOURCE=..\src\TRunInfo.c
# End Source File
# Begin Source File

SOURCE=..\src\tsftp.c
# End Source File
# Begin Source File

SOURCE=..\src\TUser.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h;*.hpp"
# Begin Source File

SOURCE=..\src\Conv.h
# End Source File
# Begin Source File

SOURCE=..\src\dbtree.h
# End Source File
# Begin Source File

SOURCE=..\src\Des.h
# End Source File
# Begin Source File

SOURCE=..\src\ExchBuff.h
# End Source File
# Begin Source File

SOURCE=..\src\Histdata.h
# End Source File
# Begin Source File

SOURCE=..\src\Notify.h
# End Source File
# Begin Source File

SOURCE=..\src\NtfyRes.h
# End Source File
# Begin Source File

SOURCE=..\src\PASSWORD.H
# End Source File
# Begin Source File

SOURCE=..\src\resource.h
# End Source File
# Begin Source File

SOURCE=..\src\S_Proc.h
# End Source File
# Begin Source File

SOURCE=..\src\schedule.h
# End Source File
# Begin Source File

SOURCE=..\src\secapi.H
# End Source File
# Begin Source File

SOURCE=..\src\shell.h
# End Source File
# Begin Source File

SOURCE=..\src\TAccount.h
# End Source File
# Begin Source File

SOURCE=..\src\TEngine.h
# End Source File
# Begin Source File

SOURCE=..\src\terror.h
# End Source File
# Begin Source File

SOURCE=..\src\tlimits.h
# End Source File
# Begin Source File

SOURCE=..\src\tqueue.h
# End Source File
# Begin Source File

SOURCE=..\src\TREE_TCP.H
# End Source File
# Begin Source File

SOURCE=..\src\TreeSvr.h
# End Source File
# Begin Source File

SOURCE=..\src\TRunInfo.h
# End Source File
# Begin Source File

SOURCE=..\src\ts_com.h
# End Source File
# Begin Source File

SOURCE=..\src\TS_CONST.H
# End Source File
# Begin Source File

SOURCE=..\src\Ts_dict.h
# End Source File
# Begin Source File

SOURCE=..\src\TSCommon.h
# End Source File
# Begin Source File

SOURCE=..\src\Tsdbfutl.h
# End Source File
# Begin Source File

SOURCE=..\src\tsftp.h
# End Source File
# Begin Source File

SOURCE=..\src\tuser.h
# End Source File
# Begin Source File

SOURCE=..\src\Wmtasql.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.ico;*.bmp"
# Begin Source File

SOURCE=..\res\blank.ico
# End Source File
# Begin Source File

SOURCE=..\res\blankbmp.bmp
# End Source File
# Begin Source File

SOURCE=..\res\pause.ico
# End Source File
# Begin Source File

SOURCE=..\res\pausebmp.bmp
# End Source File
# Begin Source File

SOURCE=..\res\run.ico
# End Source File
# Begin Source File

SOURCE=..\res\run1.ico
# End Source File
# Begin Source File

SOURCE=..\res\runbmp.bmp
# End Source File
# Begin Source File

SOURCE=..\res\stop.ico
# End Source File
# Begin Source File

SOURCE=..\res\stopbmp.bmp
# End Source File
# End Group
# Begin Group "Library Files"

# PROP Default_Filter "*.lib"
# Begin Source File

SOURCE=..\wmtasql\Debug\asqlwmt.lib
# End Source File
# Begin Source File

SOURCE=..\btree\Debug\dbtree.lib
# End Source File
# Begin Source File

SOURCE=..\tscommon\Debug\tscommon.lib
# End Source File
# Begin Source File

SOURCE=..\TAccount\Debug\taccount.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# End Target
# End Project
