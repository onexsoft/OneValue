CC       = gcc
OS      := $(shell uname)
CXX      = g++
CFLAGS   = -pipe -Wall -W -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
CXXFLAGS = -std=c++0x -DSNAPPY -pipe -Wall -W -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
INCPATH  = -I./src
LINK     = g++
LFLAGS   =
LIBS     = $(SUBLIBS)  
ifeq ($(OS),Darwin)
LIBS    += -pthread
else
LIBS    += -lrt -pthread
endif
LIBS 	+= /usr/local/lib/libevent.a
LIBS    += /usr/local/lib/libevent_pthreads.a
LIBS    += /usr/local/lib/libjemalloc.a 
LIBS    += /usr/local/lib/libleveldb.a 
LIBS    += /usr/local/lib/libsnappy.a


####### Files

HEADERS = $(shell find . -name "*.h")
SOURCES = $(shell find . -name "*.cpp")
OBJECTS = $(SOURCES:%.cpp=%.o)

TARGET  = onevalue

first: all
####### Implicit rules

.SUFFIXES: .c .o .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: Makefile $(TARGET)
	rm -fr $(OBJECTS)

$(TARGET):  $(UICDECLS) $(OBJECTS) $(OBJMOC)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(OBJCOMP) $(LIBS)


clean:
	rm -f *.core


####### Compile
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
