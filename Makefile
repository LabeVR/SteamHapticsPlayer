all: range play-pcm

CXXFLAGS = -std=c++20 `pkg-config --libs --cflags hidapi`

SHARED_SRC = $(wildcard sharedSrc/*.cpp sharedSrc/*/*.cpp sharedSrc/*.c sharedSrc/*/*.c)
RANGE_SRC = $(wildcard rangeSrc/*.cpp rangeSrc/*/*.cpp rangeSrc/*.c rangeSrc/*/*.c)
PCM_SRC = $(wildcard playPCMSrc/*.cpp playPCMSrc/*/*.cpp playPCMSrc/*.c playPCMSrc/*/*.c)

range: $(RANGE_SRC) $(SHARED_SRC)
	g++ -IsharedSrc -o range $^ $(CXXFLAGS)

play-pcm: ${PCM_SRC} ${SHARED_SRC}
	g++ -IsharedSrc -municode -o play-pcm $^ $(CXXFLAGS)