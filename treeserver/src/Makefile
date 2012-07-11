include ../config.mk

TARGET = treesvr

CFLAGS += /Zp1 

%.oo: %.c
	$(CC) $(CFLAGS)  /D "TREESVR_STANDALONE" /Fo$@ $<

CSRCS = conv.c des.c exchbuff.c notify.c password.c plug-ins.c schedule.c secapi.c \
        shell.c s_proc.c tengine.c terror.c tqueue.c treesvr.c tree_tcp.c truninfo.c \
        tsftp.c tuser.c
INCLUDES = conv.h dbtree.h des.h exchbuff.h histdata.h notify.h ntfyres.h password.h \
           plug-ins.h resource.h schedule.h secapi.h shell.h s_proc.h taccount.h tengine.h \
           terror.h tinterface.h tlimits.h tplug-ins.h tqueue.h treesvr.h tree_tcp.h \
           truninfo.h tscommon.h tsdbfutl.h tsftp.h ts_com.h ts_const.h ts_dict.h tuser.h \
           wmtasql.h      
RCS = treesvr.rc

OBJS = $(CSRCS:.c=.o)
SA_OBJS = $(CSRCS:.c=.oo)
RESS = $(RCS:.rc=.res)

LIBS += tscommon.lib dbtree.lib taccount.lib asqlwmt.lib

all: $(TARGET) $(TARGET).sa

$(TARGET): $(OBJS) $(RESS)
	$(LD) $(LDFLAGS) /LIBPATH:$(ADDLIB) $(LIBS) /out:"$@.exe" $(OBJS) $(RESS)

$(TARGET).sa: $(SA_OBJS) $(RESS)
	$(LD) $(LDFLAGS) /subsystem:windows /LIBPATH:$(SA_ADDLIB) $(LIBS) /out:"$@.exe" $(SA_OBJS) $(RESS)

install: $(TARGET) $(TARGET).sa
	if [ ! -d $(DESTINATION) ]; then mkdir $(DESTINATION); fi
	if [ ! -d $(DESTINATION)/$(SERVER) ]; then mkdir $(DESTINATION)/$(SERVER); fi
	if [ ! -d $(DESTINATION)/$(STANDALONE) ]; then mkdir $(DESTINATION)/$(STANDALONE); fi

	if [ ! -d $(FULL_IMGPATH) ]; then mkdir $(FULL_IMGPATH); fi 
	install -c $(TARGET).exe $(FULL_IMGPATH)/$(TARGET).exe

	if [ ! -d $(FULL_SA_IMGPATH) ]; then mkdir $(FULL_SA_IMGPATH); fi 
	install -c $(TARGET).sa.exe $(FULL_SA_IMGPATH)/$(TARGET).exe
 
clean:
	rm -f *.o *.oo *.res *.exp *.dll *.lib *.exe
