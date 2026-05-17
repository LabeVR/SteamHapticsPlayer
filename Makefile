all: steam-haptics-singer range

CXXFLAGS = -fpermissive `pkg-config --libs --cflags libusb-1.0 hidapi`

SHARED_SRC = $(wildcard sharedSrc/*.cpp sharedSrc/*/*.cpp sharedSrc/*.c sharedSrc/*/*.c)
SINGER_SRC = $(wildcard SteamHapticsSingerSrc/*.cpp SteamHapticsSingerSrc/*/*.cpp SteamHapticsSingerSrc/*.c SteamHapticsSingerSrc/*/*.c)
RANGE_SRC = $(wildcard rangeSrc/*.cpp rangeSrc/*/*.cpp rangeSrc/*.c rangeSrc/*/*.c)

steam-haptics-singer: $(SINGER_SRC)
	g++ -o steam-haptics-singer $^ $(CXXFLAGS)

range: $(RANGE_SRC) $(SHARED_SRC)
	g++ -IsharedSrc -o range $^ $(CXXFLAGS)