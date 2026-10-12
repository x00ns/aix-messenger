CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS = -lsqlite3 -lcrypt

TARGET = aix

$(TARGET): main.o user.o database.o message.o
	$(CXX) $(CXXFLAGS) main.o user.o message.o database.o -o $(TARGET) $(LDFLAGS)

main.o: src/main.cpp includes/user.hpp
	$(CXX) $(CXXFLAGS) -c src/main.cpp

user.o: src/user.cpp includes/user.hpp
	$(CXX) $(CXXFLAGS) -c src/user.cpp

message.o: src/message.cpp includes/message.hpp
	$(CXX) $(CXXFLAGS) -c src/message.cpp

database.o: src/database.cpp includes/database.hpp
	$(CXX) $(CXXFLAGS) -c src/database.cpp

clean:
	rm -f *.o aix.db $(TARGET)
