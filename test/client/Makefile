#|* File :: Makefile
#|*
#|* Desc :: Makefile that builds a client for testing purposes
#|*

PROJECT_ROOT=../..

#########################################################################################################
# Includes ##############################################################################################
include $(PROJECT_ROOT)/Makefile.global


TARGET = client

# Principal
all: $(PROJECT_BIN)/$(TARGET)

$(PROJECT_BIN)/$(TARGET): $(TARGET)
	echo " ::Copying:: $(TARGET) -> $@"
	cp -p $(TARGET) $@
	echo "[$(TARGET)] copied."

$(TARGET): $(TARGET).cpp $(PROJECT_LIB)/liblocar.a
	echo " ::Building:: $@"
	$(CXX) $(CXXFLAGS) $(TARGET).cpp -o $@  -I $(LIB_SRC) -llocar -L $(PROJECT_LIB)
	echo "[$@] built."


clean:
	rm -fv $(TARGET)
	rm -fv $(PROJECT_BIN)/$(TARGET)

