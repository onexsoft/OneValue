CC       = gcc
CXX      = g++
CFLAGS   = -pipe -Wall -W -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
CXXFLAGS = -std=c++0x -DSNAPPY -pipe -Wall -W -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
INCPATH  = -I./src
LINK     = g++
LFLAGS   =
LIBS     = $(SUBLIBS)  
LIBS    += -lrt -pthread
LIBS 	+= /usr/local/lib/libevent.a
LIBS    += /usr/local/lib/libevent_pthreads.a
LIBS    += /usr/local/lib/libjemalloc.a 
LIBS    += /usr/local/lib/libleveldb.a 
LIBS    += /usr/local/lib/libsnappy.a
####### Output directory

OBJECTS_DIR = tmp/

####### Files

HEADERS = $(shell find . -name "*.h")
SOURCES = $(shell find . -name "*.cpp")
FILE 	= $(shell find . -name "*.cpp" -exec basename \{} .cpp \;)
OBJECTS = $(FILE:%=$(OBJECTS_DIR)%.o)


DESTDIR  =
TARGET   = onevalue

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

$(TARGET):  $(UICDECLS) $(OBJECTS) $(OBJMOC)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(OBJCOMP) $(LIBS)


clean:
	rm -f $(OBJECTS)
	rm -f *.core


####### Compile

tmp/eventloop.o: src/eventloop.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/eventloop.o src/eventloop.cpp

tmp/hash.o: src/util/hash.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/hash.o src/util/hash.cpp

tmp/logger.o: src/util/logger.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/logger.o src/util/logger.cpp

tmp/main.o: src/main.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/main.o src/main.cpp

tmp/redisproto.o: src/redisproto.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redisproto.o src/redisproto.cpp

tmp/redisproxy.o: src/redisproxy.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/redisproxy.o src/redisproxy.cpp

tmp/command.o: src/command.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/command.o src/command.cpp

tmp/tcpsocket.o: src/util/tcpsocket.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tcpsocket.o src/util/tcpsocket.cpp

tmp/tcpserver.o: src/util/tcpserver.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tcpserver.o src/util/tcpserver.cpp

tmp/thread.o: src/util/thread.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/thread.o src/util/thread.cpp

tmp/tinystr.o: src/tinyxml/tinystr.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinystr.o src/tinyxml/tinystr.cpp

tmp/tinyxml.o: src/tinyxml/tinyxml.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinyxml.o src/tinyxml/tinyxml.cpp

tmp/tinyxmlerror.o: src/tinyxml/tinyxmlerror.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinyxmlerror.o src/tinyxml/tinyxmlerror.cpp

tmp/tinyxmlparser.o: src/tinyxml/tinyxmlparser.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/tinyxmlparser.o src/tinyxml/tinyxmlparser.cpp

tmp/iobuffer.o: src/util/iobuffer.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/iobuffer.o src/util/iobuffer.cpp

tmp/locker.o: src/util/locker.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/locker.o src/util/locker.cpp

tmp/leveldb.o: src/leveldb.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/leveldb.o src/leveldb.cpp

tmp/onevaluecfg.o: src/onevaluecfg.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/onevaluecfg.o src/onevaluecfg.cpp

tmp/monitor.o: src/monitor.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/monitor.o src/monitor.cpp

tmp/top-key.o: src/top-key.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/top-key.o src/top-key.cpp

tmp/binlog.o: src/binlog.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/binlog.o src/binlog.cpp

tmp/sync.o: src/sync.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/sync.o src/sync.cpp

tmp/non-portable.o: src/non-portable.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/non-portable.o src/non-portable.cpp

tmp/t_redis.o: src/t_redis.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/t_redis.o src/t_redis.cpp

tmp/t_zset.o: src/t_zset.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/t_zset.o src/t_zset.cpp

tmp/t_hash.o: src/t_hash.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/t_hash.o src/t_hash.cpp

tmp/zsetcmdhandler.o: src/zsetcmdhandler.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/zsetcmdhandler.o src/zsetcmdhandler.cpp

tmp/hashcmdhandler.o: src/hashcmdhandler.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/hashcmdhandler.o src/hashcmdhandler.cpp

tmp/dbcopy.o: src/dbcopy.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/dbcopy.o src/dbcopy.cpp

tmp/ttlmanager.o: src/ttlmanager.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/ttlmanager.o src/ttlmanager.cpp

tmp/cmdhandler.o: src/cmdhandler.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/cmdhandler.o src/cmdhandler.cpp

tmp/t_list.o: src/t_list.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/t_list.o src/t_list.cpp

tmp/listcmdhandler.o: src/listcmdhandler.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o tmp/listcmdhandler.o src/listcmdhandler.cpp
