include ../config.mk

TARGET = tscapi

CFLAGS += /Zp1 /D "__LIBRARY__" 

%.oo: %.c
	$(CC) $(CFLAGS) /D "TREESVR_STANDALONE" /Fo$@ $<

CSRCS = des.c lzhuf.c mistring.c namedpipe.c tree_tcp.c tscapi.c      
INCLUDES = des.h dir.h exchbuff.h lzhuf.h mistring.h namedpipe.h terror.h tlimits.h \
           tree_tcp.h tscapi.h ts_com.h ts_const.h wst2mt.h      

OBJS = $(CSRCS:.c=.o)
SA_OBJS = $(CSRCS:.c=.oo)

all: $(TARGET) $(TARGET).sa

$(TARGET): $(OBJS)
	$(LD) $(LDDFLAGS) $(LIBS) /out:"$(TARGET).dll" /implib:"$(TARGET).lib" $(OBJS)

$(TARGET).sa: $(SA_OBJS)
	$(LD) $(LDDFLAGS) $(LIBS) /out:"$@.dll" /implib:"$@.lib" $(SA_OBJS)

install: $(TARGET) $(TARGET).sa
	if [ ! -d $(DESTINATION) ]; then mkdir $(DESTINATION); fi
	if [ ! -d $(DESTINATION)/$(SERVER) ]; then mkdir $(DESTINATION)/$(SERVER); fi
	if [ ! -d $(DESTINATION)/$(STANDALONE) ]; then mkdir $(DESTINATION)/$(STANDALONE); fi

	if [ ! -d $(FULL_IMGPATH) ]; then mkdir $(FULL_IMGPATH); fi 
	install -c $(TARGET).dll $(FULL_IMGPATH)/$(TARGET).dll
	install -c $(TARGET).dll $(FULL_IMGPATH)/ts_clnt.dll
	if [ ! -d $(FULL_LIBPATH) ]; then mkdir $(FULL_LIBPATH); fi 
	install -c $(TARGET).lib $(FULL_LIBPATH)/$(TARGET).lib
	install -c $(TARGET).lib $(FULL_LIBPATH)/ts_clnt.lib
	if [ ! -d $(FULL_HDRPATH) ]; then mkdir $(FULL_HDRPATH); fi 
	install -c tscapi.h $(FULL_HDRPATH)/tscapi.h
	install -c ts_com.h $(FULL_HDRPATH)/ts_com.h
	install -c ts_const.h $(FULL_HDRPATH)/ts_const.h
	install -c terror.h $(FULL_HDRPATH)/terror.h

	if [ ! -d $(FULL_SA_IMGPATH) ]; then mkdir $(FULL_SA_IMGPATH); fi 
	install -c $(TARGET).sa.dll $(FULL_SA_IMGPATH)/$(TARGET).dll
	install -c $(TARGET).sa.dll $(FULL_SA_IMGPATH)/ts_clnt.dll
	if [ ! -d $(FULL_SA_LIBPATH) ]; then mkdir $(FULL_SA_LIBPATH); fi 
	install -c $(TARGET).sa.lib $(FULL_SA_LIBPATH)/$(TARGET).lib
	install -c $(TARGET).sa.lib $(FULL_SA_LIBPATH)/ts_clnt.lib
	if [ ! -d $(FULL_SA_HDRPATH) ]; then mkdir $(FULL_SA_HDRPATH); fi 
	install -c tscapi.h $(FULL_SA_HDRPATH)/tscapi.h
	install -c ts_com.h $(FULL_SA_HDRPATH)/ts_com.h
	install -c ts_const.h $(FULL_SA_HDRPATH)/ts_const.h
	install -c terror.h $(FULL_SA_HDRPATH)/terror.h
 
clean:
	rm -f *.o *.oo *.res *.exp *.dll *.lib 
