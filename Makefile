CXX = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS = -lsqlite3

SOURCES = BibleText.cpp BibleSearch.cpp kjv.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = kjv

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)
	rm -f $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
