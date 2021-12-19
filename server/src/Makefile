#|* File :: Makefile
#|*
#|* Desc :: makefile that builds the NCS server
#|*

PROJECT_ROOT=../..

#########################################################################################################
# Includes ##############################################################################################
include $(PROJECT_ROOT)/Makefile.global
include Makefile.dd

#########################################################################################################
# Opciones g++ ##########################################################################################
I_INCS= -I lib/src/


OBJS = main.o \
       ncs/Server.o \
       ncs/Worker.o \


TARGET = server

# Principal
all: $(PROJECT_BIN)/$(TARGET)

$(PROJECT_BIN)/$(TARGET): $(TARGET)
	echo " ::Copying:: $(TARGET) -> $@"
	cp -p $(TARGET) $@
	echo "[$(TARGET)] copied."

$(TARGET): $(OBJS) $(NCS_SERVER_HDD) $(PROJECT_LIB)/liblocar.a
	echo " ::Building:: $@"
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ -llocar -L $(PROJECT_LIB)
	echo "[$@] built."

main.o: main.cpp  $(NCS_SERVER_HDD) $(LIBLOCAR_STDLOGGER_HDD) $(LIBLOCAR_COMMANDLINE_HDD)
	echo " ::Compiling:: $*.cpp --> $@"
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@ -I $(LIB_SRC)

ncs/Server.o: ncs/Server.cpp  $(NCS_SERVER_HDD) $(LIBLOCAR_EXCEPTIONS_HDD)
	echo " ::Compiling:: $*.cpp --> $@"
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@ -I $(LIB_SRC)

ncs/Worker.o: ncs/Worker.cpp  $(NCS_WORKER_HDD) $(LIBLOCAR_STRING_HDD) $(LIBLOCAR_EXCEPTIONS_HDD) $(LIBLOCAR_MD5_HDD)
	echo " ::Compiling:: $*.cpp --> $@"
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@ -I $(LIB_SRC)


clean:
	rm -fv *.o
	rm -fv ncs/*.o
	rm -fv $(TARGET)
	rm -fv $(PROJECT_BIN)/$(TARGET)

