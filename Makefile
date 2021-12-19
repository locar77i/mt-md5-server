#|* File :: Makefile
#|*
#|* Desc :: makefile that builds the NCS server whole project and creates its output directories
#|*

.SILENT:

PROJECT_ROOT=.

#########################################################################################################
# Includes ##############################################################################################
include $(PROJECT_ROOT)/Makefile.global

library: $(PROJECT_LIB)
	echo " ::Creating:: $@"
	cd ./liblocar/src; $(MAKE) all; [ $$? = 0 ] || exit -1; cd ..;

$(PROJECT_LIB):
	echo " ::Creating:: $@"
	mkdir -p $@

server: library $(PROJECT_BIN)
	echo " ::Creating:: $@"
	cd ./server/src; $(MAKE) all; [ $$? = 0 ] || exit -1; cd ..;

test: library $(PROJECT_BIN)
	echo " ::Creating:: $@"
	cd ./test/client; $(MAKE) all; [ $$? = 0 ] || exit -1; cd ..;

$(PROJECT_BIN):
	echo " ::Creating:: $@"
	mkdir -p $@


all: server test


clean: 
	cd ./liblocar/src; $(MAKE) clean; [ $$? = 0 ] || exit -1; cd ..;
	cd ./server/src; $(MAKE) clean; [ $$? = 0 ] || exit -1; cd ..;
	cd ./test/client; $(MAKE) clean; [ $$? = 0 ] || exit -1; cd ..;
	rm -rfv $(PROJECT_LIB) || true
	rm -rfv $(PROJECT_BIN) || true

