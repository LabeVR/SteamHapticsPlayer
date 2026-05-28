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

TritonController* c = nullptr;
ControllerFinder finder;
// if anyone is wondering why this is called aou, consult this video https://www.youtube.com/watch?v=kiFCFlAUy_8
PCM aou;

struct Args {
  bool setup = true;
  path_t filePath;
  std::string help;
};


constexpr int SAMPLES_PER_PACKET = 31;
constexpr int BYTES_PER_FRAME = 2;
constexpr int NEED_BYTES = SAMPLES_PER_PACKET * BYTES_PER_FRAME;
constexpr int SAMPLE_RATE = 8000; // steam controller only supports up to signed 8 bit, 8khz
const auto period = std::chrono::microseconds((SAMPLES_PER_PACKET * 1000000) / SAMPLE_RATE);

const std::string helpString =
    "Usage: steam-haptics-singer.exe [-s] <file path>\n"
    "  -s  Skip running the setup phase if you've already run it once and haven't restarted your controller\n";

void reset(int) {
  aou.stop();
}

template <typename ArgGetter>
Args parseArgs(int argc, ArgGetter argAt) {
  Args args;

  for (int i = 1; i < argc; ++i) {
    path_t arg = argAt(i);
    std::string argStr = std::filesystem::path(arg).string();

    if (argStr == "-s") {
      args.setup = false;
    } else if (!argStr.empty() && argStr[0] == '-') {
      args.help = helpString;
      return args;
    } else {
      args.filePath = arg;
    }
  }

  if (args.filePath.empty()) args.help = helpString;
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

  int loadResult = aou.load(args.filePath);
  if (loadResult < 0) {
    if (loadResult == -2) {
      std::cout << "ffmpeg was not found on PATH\n";
    } else {
#ifdef _WIN32
      std::wcout << L"ffmpeg could not load " << args.filePath << L"\n";
#else
      std::cout << "ffmpeg could not load " << args.filePath << "\n";
#endif
    }
    return 1;
  }

  signal(SIGINT, reset);

  if (args.setup) c->setupPCMStreaming();

  byte primeBuf[NEED_BYTES];
  int pr = aou.getBytes(primeBuf, NEED_BYTES);
  (void)pr;

  auto nextPacketTime = std::chrono::steady_clock::now();

  std::cout << "Playing audio...\n";
  while (true) {
    byte tmp[NEED_BYTES];
    int r = aou.getBytes(tmp, NEED_BYTES);
    if (r <= 0) break;
    if (r < NEED_BYTES) std::memset(tmp + r, 0, NEED_BYTES - r); // s8 silence = 0

    byte buff[64] = {0};
    buff[0] = 0x88; //cmd
    buff[1] = 31; //length var

    for (int i = 0; i < SAMPLES_PER_PACKET; i++) {
      byte left = tmp[i * 2];
      byte right = tmp[i * 2 + 1];
      buff[2 + i] = left;
      buff[33 + i] = right;
    }

    c->sendRaw(buff, 64);

    
    nextPacketTime += period;
    while (std::chrono::steady_clock::now() < nextPacketTime) {}
  }

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