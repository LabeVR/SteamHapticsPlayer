all: range steam-haptics-player

ifeq ($(OS),Windows_NT)
HIDAPI_PKG ?= hidapi
UNICODE_FLAG ?= -municode
else
HIDAPI_PKG ?= hidapi-hidraw
UNICODE_FLAG ?=
endif

CXXFLAGS = -std=c++20 `pkg-config --libs --cflags $(HIDAPI_PKG)`

SHARED_SRC = $(wildcard sharedSrc/*.cpp sharedSrc/*/*.cpp sharedSrc/*.c sharedSrc/*/*.c)
RANGE_SRC = $(wildcard rangeSrc/*.cpp rangeSrc/*/*.cpp rangeSrc/*.c rangeSrc/*/*.c)
PCM_SRC = $(wildcard playPCMSrc/*.cpp playPCMSrc/*/*.cpp playPCMSrc/*.c playPCMSrc/*/*.c)

range: $(RANGE_SRC) $(SHARED_SRC)
	g++ -IsharedSrc -o range $^ $(CXXFLAGS)

steam-haptics-player: ${PCM_SRC} ${SHARED_SRC}
	g++ -IsharedSrc $(UNICODE_FLAG) -o steam-haptics-player $^ $(CXXFLAGS)