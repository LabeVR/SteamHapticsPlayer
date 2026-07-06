#include "PCM.h"
#include <ControllerFinder.h>
#include <TritonController.h>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <Utils.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

TritonController* c = nullptr;
ControllerFinder finder;
// if anyone is wondering why this is called aou, consult this video https://www.youtube.com/watch?v=kiFCFlAUy_8
PCM aou;

struct Args {
  bool setup = true;
  bool systemAudio = false;
  path_t filePath;
  std::string help;
};

constexpr int SAMPLES_PER_PACKET = 31;
constexpr int BYTES_PER_FRAME = 2;
constexpr int NEED_BYTES = SAMPLES_PER_PACKET * BYTES_PER_FRAME;
constexpr int SAMPLE_RATE = 8000; // steam controller only supports up to signed 8 bit, 8khz
const auto period = std::chrono::microseconds((SAMPLES_PER_PACKET * 1000000) / SAMPLE_RATE);

const std::string helpString =
  "Usage: steam-haptics-player.exe [-s] [--system-audio] [file path]\n"
  "  --system-audio  Capture the PC's default output through ffmpeg loopback instead of playing a file\n"
    "  -s  Skip running the setup phase if you've already run it once and haven't restarted your controller\n";

#ifndef _WIN32
class StdinRawModeGuard {
public:
  StdinRawModeGuard() {
    if (tcgetattr(STDIN_FILENO, &originalAttributes) != 0) return;
    newAttributes = originalAttributes;
    newAttributes.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
    newAttributes.c_cc[VMIN] = 0;
    newAttributes.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newAttributes) == 0) {
      active = true;
    }
  }

  ~StdinRawModeGuard() {
    if (active) tcsetattr(STDIN_FILENO, TCSANOW, &originalAttributes);
  }

private:
  termios originalAttributes{};
  termios newAttributes{};
  bool active = false;
};

bool quitKeyPressed() {
  fd_set readFds;
  FD_ZERO(&readFds);
  FD_SET(STDIN_FILENO, &readFds);

  timeval timeout{};
  int ready = select(STDIN_FILENO + 1, &readFds, nullptr, nullptr, &timeout);
  if (ready <= 0 || !FD_ISSET(STDIN_FILENO, &readFds)) return false;

  char ch = 0;
  ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
  return bytesRead == 1 && (ch == 'q' || ch == 'Q');
}
#else
bool quitKeyPressed() {
  if (!_kbhit()) return false;
  int ch = _getch();
  return ch == 'q' || ch == 'Q';
}
#endif

void reset(int) {
  // aou.stop();
}

template <typename ArgGetter>
Args parseArgs(int argc, ArgGetter argAt) {
  Args args;

  for (int i = 1; i < argc; ++i) {
    path_t arg = argAt(i);
    std::string argStr = std::filesystem::path(arg).string();

    if (argStr == "-s") {
      args.setup = false;
    } else if (argStr == "--system-audio") {
      args.systemAudio = true;
    } else if (!argStr.empty() && argStr[0] == '-') {
      args.help = helpString;
      return args;
    } else {
      args.filePath = arg;
    }
  }

  if (args.filePath.empty() && !args.systemAudio) args.help = helpString;
  return args;
}

int runPlayer(const Args& args) {
  if (!args.help.empty()) {
    std::cout << helpString;
    return 1;
  }

  SteamController* cont = finder.getController();
  if (cont == nullptr) return 1;
  if (cont->type == ControllerType::Triton) c = static_cast<TritonController*>(cont);
  if (c == nullptr) return 1;

  int loadResult = aou.load(args.filePath, args.systemAudio);
  if (loadResult < 0) {
    if (loadResult == -2) {
      std::cout << "ffmpeg was not found on PATH\n";
    } else if (loadResult == -3) {
      std::cout << "system audio capture is only supported on Windows in this build\n";
    } else {
#ifdef _WIN32
      if (args.systemAudio) {
        std::cout << "ffmpeg could not open the system audio loopback source\n";
      } else {
        std::wcout << L"ffmpeg could not load " << args.filePath << L"\n";
      }
#else
      std::cout << "ffmpeg could not load input\n";
#endif
    }
    return 1;
  }

  signal(SIGINT, reset);

  if (args.setup) c->setupPCMStreaming();

#ifndef _WIN32
  StdinRawModeGuard stdinRawModeGuard;
#endif

  if (!args.systemAudio) {
    byte primeBuf[NEED_BYTES];
    int pr = aou.getBytes(primeBuf, NEED_BYTES);
    (void)pr;
  }

  auto nextPacketTime = std::chrono::steady_clock::now();

  if (args.systemAudio) {
    std::cout << "Streaming system audio...\n";
  } else {
    std::cout << "Playing audio...\n";
  }

  MsgHapticPCMStereo packet;
  int totalSteps = args.systemAudio ? 0 : static_cast<int>(aou.fileSize > NEED_BYTES ? aou.fileSize - NEED_BYTES : 0);
  std::string start = args.systemAudio ? "Streaming: " : "Playing: ";
  Utils::ProgressHelper progress(totalSteps, &start, NEED_BYTES, Utils::Mode::TIME);

  while (true) {
    byte tmp[NEED_BYTES];
    int r = aou.getBytes(tmp, NEED_BYTES);
    if (r <= 0) break;
    if (r < NEED_BYTES) std::memset(tmp + r, 0, NEED_BYTES - r); // s8 silence = 0

    packet.length = 31;
 
    for (int i = 0; i < SAMPLES_PER_PACKET; i++) {
      byte left = tmp[i * 2];
      byte right = tmp[i * 2 + 1];
      packet.left[i] = left;
      packet.right[i] = right;
    }

    c->sendPCMStereo(&packet);
  if (!args.systemAudio) progress.step();
    nextPacketTime += period;

    if (quitKeyPressed()) {
      std::cout << "\nQuit requested.\n";
      break;
    }

    while (std::chrono::steady_clock::now() < nextPacketTime) {}
  }
  
  std::cout << std::endl;

  reset(0);
  return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
  Args args = parseArgs(argc, [&](int index) -> path_t {
    return std::wstring(argv[index]);
  });
  return runPlayer(args);
}
#else
int main(int argc, char* argv[]) {
  Args args = parseArgs(argc, [&](int index) -> path_t {
    return std::filesystem::path(argv[index]).string();
  });
  return runPlayer(args);
}
#endif