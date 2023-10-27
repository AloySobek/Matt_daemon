# Variables
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++11
INCLUDES = -I./src

# Sources and Objects for Matt_daemon
MD_SOURCES = src/main.cpp src/Matt_daemon.cpp src/Tintin_reporter.cpp
MD_OBJECTS = $(MD_SOURCES:.cpp=.o)

# Sources and Objects for Ben_AFK
BA_SOURCES = src/client.cpp src/Tintin_reporter.cpp
BA_OBJECTS = $(BA_SOURCES:.cpp=.o)

# Default target
all: Matt_daemon Ben_AFK

Matt_daemon: $(MD_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

Ben_AFK: $(BA_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Clean targets
clean:
	rm -f $(MD_OBJECTS) $(BA_OBJECTS)

fclean:
	rm -f $(MD_OBJECTS) $(BA_OBJECTS)
	rm -f Matt_daemon Ben_AFK

re: fclean all

.PHONY: all clean fclean re
