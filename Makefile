
TARGET_FOLDER    = ./output
EXECUTABLE       = video2pic
SRC_FILES        = video2pic.cpp
LINK_LIBS        = -lavformat -lavcodec -lavutil -lswscale -lopencv_highgui -lopencv_core
# learn -D__STDC_FORMAT_MACROS, see http://blog.csdn.net/win_lin/article/details/7912693
CXXFLAGS        += -g -D__STDC_FORMAT_MACROS 

all:
	mkdir -p $(TARGET_FOLDER)
	g++ $(SRC_FILES) $(CXXFLAGS) $(LINK_LIBS) -o $(EXECUTABLE)

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(TARGET_FOLDER)
