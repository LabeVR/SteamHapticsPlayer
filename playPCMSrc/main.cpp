#include "PCM.h"
#include <ControllerFinder.h>
#include <TritonController.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>

TritonController* c = nullptr;
ControllerFinder finder;

constexpr int SAMPLES_PER_PACKET = 31;
constexpr int BYTES_PER_FRAME = 2;
constexpr int NEED_BYTES = SAMPLES_PER_PACKET * BYTES_PER_FRAME;
constexpr int SAMPLE_RATE = 8000;
const auto period = std::chrono::microseconds(
    (SAMPLES_PER_PACKET * 1000000) / SAMPLE_RATE);

int main(int argc, char* argv[]) {
  SteamController* cont = finder.getController();
  if (cont == nullptr) return 1;

  if (cont->type == ControllerType::Triton)
    c = static_cast<TritonController*>(cont);
  if (c == nullptr) return 1;

  if (argc < 2) return 1;
  PCM* aou = new PCM(std::string(argv[1]));

  std::cout << "Running setup for pcm streaming! You may hear some weird noises" << std::endl;
  uint8_t channels[] = {1, 2, 3, 4, 5};
  uint8_t params[] = {0, 1, 2, 4, 8, 16, 32, 64};
  for (uint8_t ch : channels) {
    for (uint8_t p : params) {
      byte enable[4] = {0x86, 0x02, ch, p};
      c->sendRaw(enable, 4);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      for (int rep = 0; rep < 50; rep++) {
        byte buff[64] = {0};
        buff[0] = 0x88;
        buff[1] = 31;
        for (int i = 0; i < 31; i++) {
          uint8_t sample = ((i / 4) % 2) ? 0xFF : 0x00;
          buff[2 + i] = sample;
          buff[33 + i] = sample;
        }
        c->sendRaw(buff, 64);
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
      }
    }
  }
  std::cout << "Setup finished. Playing audio..." << std::endl;

  auto next_packet = std::chrono::steady_clock::now();

  while (true) {
    uint8_t tmp[NEED_BYTES];
    int r = aou->getBytes(tmp, NEED_BYTES);
    if (r <= 0) break;
    if (r < NEED_BYTES) std::memset(tmp + r, 0, NEED_BYTES - r); // s8 silence = 0

    byte buff[64] = {0};
    buff[0] = 0x88;
    buff[1] = 31;

    for (int i = 0; i < SAMPLES_PER_PACKET; i++) {
      uint8_t left = tmp[i * 2];
      uint8_t right = tmp[i * 2 + 1];
      buff[2 + i] = right;
      buff[33 + i] = left;
    }

    c->sendRaw(buff, 64);

    next_packet += period;
    while (std::chrono::steady_clock::now() < next_packet) {
    }
  }

  delete aou;
  c->close();
  return 0;
}