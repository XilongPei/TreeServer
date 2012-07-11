include ../config.mk

TARGET = sql_dio asqlwmt asqlclt

#
# asqlwmt & asqlclt must build with option 1 byte alignment(/Zp1)
#
CFLAGS += /Zp1 /D "ASQL_NT" 

%.oo: %.c
	$(CC) $(CFLAGS) /D "ASQLCLT" /Fo$@ $<

CSRCS = arrayutl.c ascii_1.c ascii_2.c ascii_3.c ascii_4.c ascii_5.c ascii_6.c ascii_7.c askact.c \
		askact2.c askactuv.c asql_opt.c asqlana.c asqlerr.c asqlmain.c asqlpath.c \
		asqlutl.c bc2watc.c btree.c btreeadd.c btreeext.c btreewmt.c cfg_fast.c charutl.c \
		dbftotal.c db_s_ndx.c dbs_util.c des.c dio.c diodbt.c dioext.c diofile.c exchbuff.c \
		filetool.c histdata.c hzth.c macro.c memutl.c mistools.c mistring.c mxccfg.c \
		ndx_man.c odbc_dio.c parabrk.c rjio.c str_gram.c strutl.c syscall.c t_int64.c \
		t_lock_r.c tabtools.c treepara.c treevent.c ts_dict.c tsdbfutl.c wmtasql.c wst2mt.c \
		xexp.c xexpfun.c

INCLUDES = arrayutl.h asqlana.h asqlerr.h asqlutl.h blob.h btree.h btreeadd.h btreeext.h \
		cfg_fast.h datetime.h dbftotal.h decimal.h dio.h diodbt.h dioext.h dir.h exchbuff.h filetool.h \
		hzth.h idbase.h locator.h macro.h memutl.h mistools.h mistring.h odbc_dio.h \
		parabrk.h pubvar.h readcfg.h sqlca.h sqlda.h sqlhdr.h sqlproto.h strutl.h syscall.h \
		tabtools.h taccount.h terror.h tlimits.h treepara.h treevent.h ts_const.h ts_dict.h \
		tscommon.h tsdbfutl.h util.h uvutl.h value.h wmtasql.h wst2mt.h xexp.h

RCS = Script1.rc

INCLUDES = dbtree.h dir.h memutl.h mistring.h
OBJS = $(CSRCS:.c=.o)
CLT_OBJS = $(CSRCS:.c=.oo)
RESS = $(RCS:.rc=.res)

LIBS += dbtree.lib

all: $(TARGET)

asqlwmt: sql_dio $(OBJS)
	$(LD) $(LDDFLAGS) /LIBPATH:$(ADDLIB) $(LIBS) /out:"$@.dll" /implib:"$@.lib" $(OBJS) taccount.lib tscommon.lib sql_dio.lib 

asqlclt: $(CLT_OBJS)
	$(LD) $(LDDFLAGS) /LIBPATH:$(ADDLIB) $(LIBS) /out:"$@.dll" /implib:"$@.lib" $(CLT_OBJS) sql_dio.lib

sql_dio: idbase_b.o
	$(LD) $(LDDFLAGS) /LIBPATH:$(ADDLIB) $(LIBS) /out:"$@.dll" /implib:"$@.lib" idbase_b.o

install: $(TARGET)
	if [ ! -d $(DESTINATION) ]; then mkdir $(DESTINATION); fi
	if [ ! -d $(DESTINATION)/$(SERVER) ]; then mkdir $(DESTINATION)/$(SERVER); fi
	if [ ! -d $(DESTINATION)/$(STANDALONE) ]; then mkdir $(DESTINATION)/$(STANDALONE); fi

	if [ ! -d $(FULL_IMGPATH) ]; then mkdir $(FULL_IMGPATH); fi 
	for f in $(TARGET); do	\
		install -c $$f.dll $(FULL_IMGPATH)/$$f.dll; \
	done
	if [ ! -d $(FULL_LIBPATH) ]; then mkdir $(FULL_LIBPATH); fi 
	for f in $(TARGET); do	\
		install -c $$f.lib $(FULL_LIBPATH)/$$f.lib; \
	done

	if [ ! -d $(FULL_SA_IMGPATH) ]; then mkdir $(FULL_SA_IMGPATH); fi 
	for f in $(TARGET); do	\
		install -c $$f.dll $(FULL_SA_IMGPATH)/$$f.dll; \
	done
	if [ ! -d $(FULL_SA_LIBPATH) ]; then mkdir $(FULL_SA_LIBPATH); fi 
	for f in $(TARGET); do	\
		install -c $$f.lib $(FULL_SA_LIBPATH)/$$f.lib; \
	done

	if [ ! -d $(FULL_HDRPATH) ]; then mkdir $(FULL_HDRPATH); fi 
	install -c wmtasql.h $(FULL_HDRPATH)/wmtasql.h
	install -c tsdbfutl.h $(FULL_HDRPATH)/tsdbfutl.h

	if [ ! -d $(FULL_SA_HDRPATH) ]; then mkdir $(FULL_SA_HDRPATH); fi 
	install -c wmtasql.h $(FULL_SA_HDRPATH)/wmtasql.h
	install -c tsdbfutl.h $(FULL_SA_HDRPATH)/tsdbfutl.h

clean:
	rm -f *.o *.oo *.res *.exp *.lib *.dll
