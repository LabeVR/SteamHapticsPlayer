#include "PCM.h"
#include <ControllerFinder.h>
#include <TritonController.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>
#include <iostream>
#include <iomanip>
#include <Constants.h>
TritonController* c = nullptr;
ControllerFinder finder;
PCM aou;

struct Args {
  bool setup = true;
  std::wstring filePath;
  std::wstring help;
};

constexpr int SAMPLES_PER_PACKET = 31;
constexpr int BYTES_PER_FRAME = 2;
constexpr int NEED_BYTES = SAMPLES_PER_PACKET * BYTES_PER_FRAME;
constexpr int SAMPLE_RATE = 8000; // steam controller only supports up to 8khz
const auto period = std::chrono::microseconds(
    (SAMPLES_PER_PACKET * 1000000) / SAMPLE_RATE);

std::wstring helpString = 
          L"Usage: play-pcm.exe [-s] <file path>\n"
          L"  -s  Skip running the setup phase (if you have already run it once and havent restarted your controller)\n";
Args parseArgs(int argc, wchar_t* argv[]) {
  Args args;

  for (int i = 1; i < argc; ++i) {
    std::wstring arg = argv[i];

    if (arg == L"-s") {
      args.setup = false;
    } else if (!arg.empty() && arg[0] == L'-') {
      args.help = helpString;
      return args;
    } else {
      args.filePath = arg;
    }
  }

  if (args.filePath.empty()) {
    args.help = helpString;
  }

  return args;
}


void reset(int) {
  aou.stop();
  return;
}


int wmain(int argc, wchar_t* argv[]) {
  Args args = parseArgs(argc, argv);
  if (!args.help.empty()) {
    std::wcout << args.help;
    return 1;
  }

  SteamController* cont = finder.getController();
  if (cont == nullptr) return 1;

  if (cont->type == ControllerType::Triton)
    c = static_cast<TritonController*>(cont);
  if (c == nullptr) return 1;

  if (aou.load(args.filePath) < 0) {
    std::wcout << L"ffmpeg could not load " << args.filePath;
    return 1;
  }

  if (args.setup) c->setupPCMStreaming();

  // remove first chunk to avoid cutting out first little bit of audio
  byte primeBuf[NEED_BYTES];
  int pr = aou.getBytes(primeBuf, NEED_BYTES);
  (void)pr;

  auto next_packet = std::chrono::steady_clock::now();

  std::cout << "Playing audio..." << std::endl;

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

    next_packet += period;
    while (std::chrono::steady_clock::now() < next_packet) {}
  }
  signal(SIGINT, reset);
  reset(0);
  return 0;
}