#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#

# define the Cpp compiler to use
CXX = g++

# define any compile-time flags
# CXXFLAGS	:= -std=c++2a -Wall -Wextra -g -pthread

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
# LIBS = -lldap -llber

CXXFLAGS=-g -Wall -Wextra -O -std=c++2a -I /usr/local/include/gtest/ -pthread
LIBS=-lldap -llber
GTEST=/usr/local/lib/libgtest.a

all: server client
clean:
	rm -fr obj/* target/*

rebuild: clean all

server: target/server
client: target/client

obj/mail.o: src/utils/Mail.cpp
	$(CXX) $(CXXFLAGS) -o obj/mail.o src/utils/Mail.cpp -c

obj/logger.o: src/utils/Logger.cpp
	$(CXX) $(CXXFLAGS) -o obj/logger.o src/utils/Logger.cpp -c

obj/server.o: src/server/Server.cpp
	$(CXX) $(CXXFLAGS) -o obj/server.o src/server/Server.cpp -c

obj/commhandler.o: src/server/CommHandler.cpp
	$(CXX) $(CXXFLAGS) -o obj/commhandler.o src/server/CommHandler.cpp -c

obj/server_main.o: src/server/main.cpp
	$(CXX) $(CXXFLAGS) -o obj/server_main.o src/server/main.cpp -c

obj/getpw.o: src/client/getpw.cpp
	$(CXX) $(CXXFLAGS) -o obj/getpw.o src/client/getpw.cpp -c

target/server: obj/server.o obj/commhandler.o obj/server_main.o obj/logger.o obj/mail.o
	$(CXX) $(CXXFLAGS) -o target/server obj/mail.o obj/commhandler.o obj/server.o obj/logger.o obj/server_main.o $(LIBS)

target/client: src/client/main.cpp src/client/getpw.cpp
	$(CXX) $(CXXFLAGS) src/client/main.cpp src/client/getpw.cpp -o target/client $(LIBS)
