UNAME = $(shell uname)

CC = g++

OPTS += -g
OPTS += -O3
OPTS += -std=c++0x

CPPFLAGS += $(OPTS)

#WarnAll or warn none. It's your choice...
CPPFLAGS += -Wall
#CPPFLAGS += -w

CPPFLAGS += -I./ -I./include

LIBCONFIGURATION = -L./lib -lconfiguration

LINK_LIBS += -lboost_regex

CPPFLAGS += $(LINK_LIBS)

CPPFILES += configuration.cpp

SOURCES = $(addprefix ./src/,  $(CPPFILES))

OBJECTS = $(addprefix ./build/,  $(CPPFILES:.cpp=.o)) 

CONFIGURATION_LIB = lib/libconfiguration.a

all: setup $(CONFIGURATION_LIB) 

setup:
	mkdir -p build
	mkdir -p lib

$(CONFIGURATION_LIB): $(OBJECTS)
	ar -r $(CONFIGURATION_LIB) $(OBJECTS)

build/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

.c.o:
	$(CXX) $(CPPFLAGS) -c $<

clean:
	\rm -rf build lib
