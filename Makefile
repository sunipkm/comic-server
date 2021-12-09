CXX = g++

EDCXXFLAGS:= -Wall -O2 -std=c++11 -I drivers/ -I include/ -I network/ $(CXXFLAGS) -DDEFINE_WEAK
EDLDFLAGS:= -lpthread -lcfitsio$(LDFLAGS)

ifeq ($(OS),Windows_NT)
    CCFLAGS += -D WIN32
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CCFLAGS += -D AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CCFLAGS += -D AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CCFLAGS += -D IA32
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    UNAME_P := $(shell uname -p)
	EDLDFLAGS += -lm -lpthread
    ifeq ($(UNAME_S),Linux)
        ifeq ($(UNAME_P),x86_64)
        	EDLDFLAGS += -L lib/linux/64/NoFlyCapture -latikcameras
    	endif
    	ifeq ($(filter %86,$(UNAME_P)),)
        	CCFLAGS += -D IA32
    	endif
    	ifeq ($(filter arm%,$(UNAME_P)),)
        	EDLDFLAGS += -L lib/ARM/32/NoFlyCapture -latikcameras
    	endif
    endif
    ifeq ($(UNAME_S),Darwin)
        EDLDFLAGS += -L lib/macos -latikcameras
    endif
endif

CPPOBJS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

all: $(CPPOBJS)
	$(CXX) -o atiktest.out $(CPPOBJS) $(EDLDFLAGS)

%.o: %.cpp
	$(CXX) $(EDCXXFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	rm -vf $(CPPOBJS)
	rm -vf *.out
	rm -vf *.jpg