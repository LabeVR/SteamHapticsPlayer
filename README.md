# Steam Haptics Player

This project allows stereo audio to be streamed from a file to the haptics of a Steam controller (2026), practically allowing anything to be played on it's haptic motors.

## How To

**Supported device:** Steam Controller (2026). Other controllers are unsupported.
**Requirements:** FFmpeg on PATH

#### On Windows
1. Open Powershell
2. (skip if already installed) Install FFmpeg with `winget install ffmpeg`
3. Run the program with `.\steam-haptics-player [AUDIO_FILE]`
4. Profit!


#### On Linux (not available for the moment)
1. Right click inside the folder
2. Click "Open in Terminal"
3. (skip if already installed) Install FFmpeg with a package manager of your choice 
4. Type `chmod +x steam-haptics-singer` to make the program executable
5. Run the program with `.\steam-haptics-player [AUDIO_FILE]`
6. Profit!




### Usage from command prompt:
	Usage: play-pcm.exe [-s] <file path>
          -s  Skip running the setup phase (if you have already run it once and havent restarted your controller)

### Troubleshooting
- **No audio:**  
  - Ensure you ran the setup phase (do not use `-s`) at least once since the controller was last started.
- **Garbled audio:**  
  - Re-run without `-s`. If issues persist, restart the controller and run setup again.
- **Loud static immediately on launch:**  
  - Restart the controller and try again.
- **ffmpeg not found / decode errors:**  
  - Install `ffmpeg` and ensure it is on your `PATH` (`winget install ffmpeg` on Windows).

## Compiling
**Build (Linux / WSL)**

*Note that currently there is no linux build as i catered to windows being bad and such used wide strings/etc, may honestly revert that for linux support*
```bash
sudo apt install build-essential pkg-config libhidapi-dev ffmpeg
make
```

**Build (Windows, MSYS2 UCRT64)**
```bash
pacman -Syu

pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-hidapi \
          mingw-w64-ucrt-x86_64-ffmpeg \
          mingw-w64-ucrt-x86_64-make \
          mingw-w64-ucrt-x86_64-pkgconf

make
```
## Changelog

[v1.0.0]
* Initial release!


I spent alot of time and effort reverse engineering the Steam controller's firmware to find the HID commands needed to do this so i would appreciate the stars very much!

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/N4N6145I0V)

I was also heavily inspired to do this by CrazyCritic89's [SteamHapticsSinger](https://github.com/CrazyCritic89/SteamHapticsSinger). I suggest checking it out if you wish to play MIDI files on the steam controller!
(they honestly can sound better than this at times)