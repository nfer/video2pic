
TARGET_FOLDER    = ./output
EXECUTABLE       = video2pic
SRC_FILES        = video2pic.cpp
LINK_LIBS        = -lavformat -lavcodec -lavutil -lswscale -lopencv_highgui -lopencv_core
CXXFLAGS        += -g

all:
	mkdir -p $(TARGET_FOLDER)
	g++ $(SRC_FILES) $(CXXFLAGS) $(LINK_LIBS) -o $(EXECUTABLE)

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(TARGET_FOLDER)
