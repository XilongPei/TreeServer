include ../config.mk

TARGET = tsado

CFLAGS += /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /c /I $(ADDINC)
RCFLAGS += /D "_AFXDLL"
LDDFLAGS += /subsystem:windows /libpath:$(ADDLIB)
MTLFLAGS = 

CSRCS = aboutbox.cpp command.cpp stdafx.cpp tsado.cpp
INCLUDES = aboutbox.h command.h l.h resource.h stdafx.h
RCS = tsado.rc
DEFS = tsado.def
IDLS = tsado.idl
OTHER = dlldatax.c dlldatax.h adosvr.ico command.rgs

OBJS = $(CSRCS:.cpp=.o)
RESS = $(RCS:.rc=.res)
TLBS = $(IDLS:.idl=.tlb)

LIBS += tscapi.lib

all: $(TARGET)

$(TARGET): $(OBJS) $(RESS)
	$(LD) $(LDDFLAGS) $(LIBS) /DEF:$(DEFS) /out:"$@.dll" /implib:"$@.lib" $(OBJS) $(RESS)

$(OBJS): tsado.h tsado_i.c

tsado.h tsado_i.c: $(TLBS)

install: $(TARGET)
	if [ ! -d $(DESTINATION) ]; then mkdir $(DESTINATION); fi
	if [ ! -d $(DESTINATION)/$(SERVER) ]; then mkdir $(DESTINATION)/$(SERVER); fi

	if [ ! -d $(FULL_IMGPATH) ]; then mkdir $(FULL_IMGPATH); fi 
	install -c $(TARGET).dll $(FULL_IMGPATH)/$(TARGET).dll
	install -c $(TARGET).tlb $(FULL_IMGPATH)/$(TARGET).tlb
	if [ ! -d $(FULL_LIBPATH) ]; then mkdir $(FULL_LIBPATH); fi 
	install -c $(TARGET).lib $(FULL_LIBPATH)/$(TARGET).lib
 
clean:
	rm -f *.o *.exp *.res *.tlb *.lib *.dll
