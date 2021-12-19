#|* File :: Makefile
#|*
#|* Desc :: makefile that builds a reusable utility library
#|*

PROJECT_ROOT=../..

#########################################################################################################
# Includes ##############################################################################################
include $(PROJECT_ROOT)/Makefile.global
include Makefile.dd

#########################################################################################################
# Opciones g++ ##########################################################################################
I_INCS= -I lib/src/


OBJS = lcr/md5.o \
       lcr/StdLogger.o


TARGET = liblocar.a

# Principal
all: $(PROJECT_LIB)/$(TARGET)

$(PROJECT_LIB)/$(TARGET): $(TARGET)
	echo " ::Copying:: $(TARGET) -> $@"
	cp -p $(TARGET) $@
	echo "[$(TARGET)] copied."

$(TARGET): $(OBJS)
	echo " ::Building :: $@"
	ar -r $@ $(OBJS)
	echo "[$@] built."

lcr/md5.o: lcr/md5.cpp  
	echo " ::Compiling:: $*.cpp --> $@"
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@

lcr/StdLogger.o: lcr/StdLogger.cpp  $(LIBLOCAR_STRING_HDD)
	echo " ::Compiling:: $*.cpp --> $@"
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@


clean:
	rm -fv lcr/*.o
	rm -fv $(TARGET)
	rm -fv $(PROJECT_LIB)/$(TARGET)

