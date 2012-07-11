include ../config.mk

TARGET = tscommon

CSRCS = tscommon.c
INCLUDES = tscommon.h
OBJS = $(CSRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDDFLAGS) $(LIBS) /out:"$(TARGET).dll" /implib:"$(TARGET).lib" $(OBJS)

install: $(TARGET)
	if [ ! -d $(DESTINATION) ]; then mkdir $(DESTINATION); fi
	if [ ! -d $(DESTINATION)/$(SERVER) ]; then mkdir $(DESTINATION)/$(SERVER); fi
	if [ ! -d $(DESTINATION)/$(STANDALONE) ]; then mkdir $(DESTINATION)/$(STANDALONE); fi

	if [ ! -d $(FULL_IMGPATH) ]; then mkdir $(FULL_IMGPATH); fi 
	install -c $(TARGET).dll $(FULL_IMGPATH)/$(TARGET).dll
	if [ ! -d $(FULL_LIBPATH) ]; then mkdir $(FULL_LIBPATH); fi 
	install -c $(TARGET).lib $(FULL_LIBPATH)/$(TARGET).lib
	if [ ! -d $(FULL_HDRPATH) ]; then mkdir $(FULL_HDRPATH); fi 
	install -c tscommon.h $(FULL_HDRPATH)/tscommon.h

	if [ ! -d $(FULL_SA_IMGPATH) ]; then mkdir $(FULL_SA_IMGPATH); fi 
	install -c $(TARGET).dll $(FULL_SA_IMGPATH)/$(TARGET).dll
	if [ ! -d $(FULL_SA_LIBPATH) ]; then mkdir $(FULL_SA_LIBPATH); fi 
	install -c $(TARGET).lib $(FULL_SA_LIBPATH)/$(TARGET).lib
	if [ ! -d $(FULL_SA_HDRPATH) ]; then mkdir $(FULL_SA_HDRPATH); fi 
	install -c tscommon.h $(FULL_SA_HDRPATH)/tscommon.h
 
clean:
	rm -f *.o *.res *.exp *.lib *.dll
