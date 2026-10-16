# Builds the three versions. On Windows/Linux (class setup): g++ -fopenmp.
# On macOS, Apple clang needs the libomp library from Homebrew (brew install libomp).
CXX = g++
CXXFLAGS = -O2 -std=c++17 -Wall -fopenmp
LDFLAGS = -fopenmp

ifeq ($(shell uname -s),Darwin)
    # brew may not be in the PATH (e.g. when the notebook kernel runs make): fall back to the
    # default Homebrew locations on Apple Silicon and Intel Macs
    LIBOMP := $(shell brew --prefix libomp 2>/dev/null)
    ifeq ($(LIBOMP),)
        LIBOMP := $(firstword $(wildcard /opt/homebrew/opt/libomp /usr/local/opt/libomp))
    endif
    # use the SDK's C++ headers explicitly (avoids a stale header folder left
    # behind by old Command Line Tools installs)
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
