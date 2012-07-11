DIRS = tscommon taccount btree wmtasql tscapi src tsado

all:
	@for i in $(DIRS) ; do \
		make -C $$i all ; \
		if [ $$? -ne 0 ]; then break ; fi ; \
	done

install:
	for i in $(DIRS) ; do \
		make -C $$i install ; \
		if [ $$? -ne 0 ]; then break ; fi ; \
	done

clean:
	for i in $(DIRS) ; do \
		make -C $$i clean ; \
	done


