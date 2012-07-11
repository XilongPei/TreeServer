# All path must be absolute path.
DESTINATION = C:/MyProjects/TreeServer/exports

SERVER = Server
STANDALONE = StandAlone

IMGPATH = bin
LIBPATH = lib
HDRPATH = include

# INTERNAL USE NOT EDIT FOLOWING VAR
FULL_IMGPATH = $(DESTINATION)/$(SERVER)/$(IMGPATH)
FULL_LIBPATH = $(DESTINATION)/$(SERVER)/$(LIBPATH)
FULL_HDRPATH = $(DESTINATION)/$(SERVER)/$(HDRPATH)

FULL_SA_IMGPATH = $(DESTINATION)/$(STANDALONE)/$(IMGPATH)
FULL_SA_LIBPATH = $(DESTINATION)/$(STANDALONE)/$(LIBPATH)
FULL_SA_HDRPATH = $(DESTINATION)/$(STANDALONE)/$(HDRPATH)

ADDINC = $(FULL_HDRPATH)
ADDLIB = $(FULL_LIBPATH)

SA_ADDINC = $(FULL_SA_HDRPATH)
SA_ADDLIB = $(FULL_SA_LIBPATH)

# compilers
CC = cl.exe
LD = link.exe
RC = rc.exe
IDLC = midl.exe

# default flags
CFLAGS = /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MT" /D "_MBCS" /D "_NTSDK" /c 
LDFLAGS = /nologo /incremental:no /pdb:NONE /machine:I386 
LDDFLAGS = /nologo /dll /incremental:no /pdb:NONE /machine:I386
RCFLAGS = /l 0x804 /d "NDEBUG" 
IDLFLAGS = /Oicf 

#default libraries
LIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib \
	shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib \
	netapi32.lib wsock32.lib

#default shell
SHELL = /bin/sh

# Ruler
%.o: %.c
	$(CC) $(CFLAGS) /Fo$@ $<

%.o: %.cpp
	$(CC) $(CFLAGS) /Fo$@ $<

%.tlb: %.idl
	$(IDLC) /tlb $@ /h $(basename $<).h /iid $(basename $<)_i.c $(IDLFLAGS) $<

%.res: %.rc
	$(RC) $(RCFLAGS) /fo$@ $<