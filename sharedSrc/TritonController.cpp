#include "TritonController.h"
#include "Constants.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>

TritonController::TritonController(hid_device* handle) : SteamController(ControllerType::Triton) {
  this->hid_handle = handle;
}

void TritonController::close() {
  hid_close(hid_handle);
}

// these 2 are basically the only thing stolen from SteamHapticsSinger
int TritonController::playNote(int channel, int note, int velocity) {
  double frequency = midiFrequency[note];
  return playFrequency(channel, frequency, velocity);
}

int TritonController::playFrequency(int channel, double frequency, int velocity) {
  byte packet[10] = {0};
  if (frequency == -1) {
    // This prevents the controller from rebooting when using rumble motors and drifting out of tune
    packet[0] = 0x81;
    packet[1] = channel;
  } else {
    int frequencyValue = static_cast<int>(frequency);
    packet[0] = 0x83;
    packet[1] = channel;
    packet[2] = velocity;
    packet[3] = frequencyValue & 0xFF;
    packet[4] = (frequencyValue >> 8) & 0xFF;
    packet[5] = 0xFF;
    // 1 = 0.25 secs (i think)
    packet[6] = 0x64;
  }
  return sendRaw(packet, sizeof(packet));
}

int TritonController::sendRaw(byte packet[], size_t length) {
  int r = hid_write(this->hid_handle, packet, length);
  if (r < 0) {
    wprintf(L"Send Error, hid_error: %ls\n", hid_error(this->hid_handle));
    exit(1);
  }
  return r;
}

void TritonController::setupPCMStreaming() {
  std::cout << "Running setup for pcm streaming! You may hear some weird noises" << std::endl;
  // not entirely sure how these 2 actually make the pcm streaming work
  // channels:
  // 0 - only left audio plays
  // 1 - only right audio plays
  // 3-5 on their own, nothing plays
  // but then 0,3,4,5 seems to sound okay??
  // idk im just leaving it at this to not risk audio quality, which they do seem to affect from testing
  // only needs to be setup once though per restart of the controller. then any pcm streamed to it will play just fine

  byte channels[] = {0, 1, 2, 3, 4, 5};
  byte params[] = {0, 1, 2, 4, 8, 16, 32, 64, 128};
  int reps = 10;

  int totalSteps = (sizeof(channels) / sizeof(channels[0])) * (sizeof(params) / sizeof(params[0])) * reps;
  int completed = 0;
  std::size_t lastStatusLen = 0;
  for (byte ch : channels) {
    for (byte p : params) {
      byte enable[4] = {0x86, 0x02, ch, p};
      sendRaw(enable, 4);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      for (int rep = 0; rep < reps; rep++) {
        byte buff[64] = {0};
        buff[0] = 0x88;
        buff[1] = 31;
        for (int i = 0; i < 31; i++) {
          byte sample = ((i / 4) % 2) ? 0xFF : 0x00;
          buff[2 + i] = sample;
          buff[33 + i] = sample;
        }
        sendRaw(buff, 64);
        // fun fact, windows likes to lie if it spent 1ms waiting when in reality it was spending >10ms
        std::this_thread::sleep_for(std::chrono::microseconds(15000));

        completed++;
        double percent = (100.0 * completed) / (double)totalSteps;
        std::ostringstream ss;
        ss << "Setup: " << completed << "/" << totalSteps << " (" << std::fixed << std::setprecision(1) << percent << "%)";
        std::string status = ss.str();
        std::cout << '\r' << status;
        if (lastStatusLen > status.size()) std::cout << std::string(lastStatusLen - status.size(), ' ');
        std::cout << std::flush;
        lastStatusLen = status.size();
      }
    }
  }
  std::cout << std::endl
            << "Setup finished." 
            << std::endl
            << "If your audio sounds good, you can skip this step with -s"
            << std::endl;
}
