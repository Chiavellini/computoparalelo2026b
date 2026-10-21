CXX = g++
CXXFLAGS = -O2 -std=c++17 -Wall -fopenmp
LDFLAGS = -fopenmp

ifeq ($(shell uname -s),Darwin)

    LIBOMP := $(shell brew --prefix libomp 2>/dev/null)
    ifeq ($(LIBOMP),)
        LIBOMP := $(firstword $(wildcard /opt/homebrew/opt/libomp /usr/local/opt/libomp))
    endif

    SDK = $(shell xcrun --show-sdk-path)
    CXX = clang++
    CXXFLAGS = -O2 -std=c++17 -Wall -Xpreprocessor -fopenmp -I$(LIBOMP)/include -isystem $(SDK)/usr/include/c++/v1
    LDFLAGS = -L$(LIBOMP)/lib -lomp
endif

all: dbscan_serial dbscan_p1 dbscan_p2

%: %.cpp common.hpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f dbscan_serial dbscan_p1 dbscan_p2
