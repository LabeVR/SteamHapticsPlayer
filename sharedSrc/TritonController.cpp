#include "TritonController.h"
#include "Constants.h"
#include <cmath>


TritonController::TritonController(hid_device *handle) : SteamController(ControllerType::Triton) {
  this->hid_handle = handle;
}

void TritonController::close() {
  hid_close(hid_handle);
}

int TritonController::playNote(int channel, int note, int velocity) {
  double frequency = midiFrequency[note];
  return playFrequency(channel, frequency, velocity);  
}

int TritonController::playFrequency(int channel, double frequency, int velocity) {
    byte packet[65] = {0};
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
