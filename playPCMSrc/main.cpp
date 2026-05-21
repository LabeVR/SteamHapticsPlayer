#include "PCM.h"
#include <ControllerFinder.h>
#include <TritonController.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>
#include <iostream>
#include <iomanip>

TritonController* c = nullptr;
ControllerFinder finder;
PCM aou;

constexpr int SAMPLES_PER_PACKET = 31;
constexpr int BYTES_PER_FRAME = 2;
constexpr int NEED_BYTES = SAMPLES_PER_PACKET * BYTES_PER_FRAME;
constexpr int SAMPLE_RATE = 8000;
const auto period = std::chrono::microseconds(
    (SAMPLES_PER_PACKET * 1000000) / SAMPLE_RATE);


void reset(int) {
  aou.stop();
  return;
}



int wmain(int argc, wchar_t* argv[]) {
  SteamController* cont = finder.getController();
  if (cont == nullptr) return 1;

  if (cont->type == ControllerType::Triton)
    c = static_cast<TritonController*>(cont);
  if (c == nullptr) return 1;

  if (argc < 2) return 1;

  if (aou.load(std::filesystem::path(argv[1]).wstring()) < 0) {
    std::wcout << "ffmpeg could not load " << argv[1];
    return 1;
  }

  //c->setupPCMStreaming();

  auto next_packet = std::chrono::steady_clock::now();

  while (true) {
    uint8_t tmp[NEED_BYTES];
    int r = aou.getBytes(tmp, NEED_BYTES);
    if (r <= 0) break;
    if (r < NEED_BYTES) std::memset(tmp + r, 0, NEED_BYTES - r); // s8 silence = 0

    byte buff[64] = {0};
    buff[0] = 0x88;
    buff[1] = 31;

    for (int i = 0; i < SAMPLES_PER_PACKET; i++) {
      uint8_t left = tmp[i * 2];
      uint8_t right = tmp[i * 2 + 1];
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