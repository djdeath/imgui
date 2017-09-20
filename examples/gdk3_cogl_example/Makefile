#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
#
# You will need Gtk+ 3.0 & Cogl
#
#   apt-get install libgtk-3-dev libcogl-dev  # Linux
#

#CXX = g++

EXE = gdk3_cogl_example
OBJS = main.o imgui_impl_gdk3_cogl.o
OBJS += ../../imgui.o ../../imgui_demo.o ../../imgui_draw.o

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = `pkg-config --libs gdk-3.0 cogl-2.0-experimental`

	CXXFLAGS = -I../../ `pkg-config --cflags gdk-3.0 cogl-2.0-experimental`
	CXXFLAGS += -Wall -Wformat -g3 -O0
	CFLAGS = $(CXXFLAGS)
endif

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $(EXE) $(OBJS) $(CXXFLAGS) $(LIBS)

clean:
	rm $(EXE) $(OBJS)
