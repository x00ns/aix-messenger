CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS = -lsqlite3 -lcrypt -lcrypto -lssl -lpthread

TARGET = aix

all: aix_server aix_client

aix_client: main.o client.o
	$(CXX) $(CXXFLAGS) main.o client.o -o aix_client

aix_server: server.o database.o message.o user.o
	$(CXX) $(CXXFLAGS) server.o database.o message.o user.o -o aix_server $(LDFLAGS)

main.o: src/main.cpp includes/user.hpp
	$(CXX) $(CXXFLAGS) -c src/main.cpp

user.o: src/user.cpp includes/user.hpp
	$(CXX) $(CXXFLAGS) -c src/user.cpp

message.o: src/message.cpp includes/message.hpp
	$(CXX) $(CXXFLAGS) -c src/message.cpp

database.o: src/database.cpp includes/database.hpp
	$(CXX) $(CXXFLAGS) -c src/database.cpp

server.o: src/server.cpp includes/protocol.hpp
	$(CXX) $(CXXFLAGS) -c src/server.cpp

client.o: src/client.cpp includes/protocol.hpp
	$(CXX) $(CXXFLAGS) -c src/client.cpp

clean:
	rm -f *.o aix.db aix_server aix_client
