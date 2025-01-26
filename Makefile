CXX = g++
CXXFLAGS = -std=c++11 -Wall
LDFLAGS = -lsqlite3

SOURCES = BibleText.cpp kjv.cpp BibleSearch.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = kjv

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
