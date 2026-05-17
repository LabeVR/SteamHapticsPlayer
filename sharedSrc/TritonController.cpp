#include "TritonController.h"
#include "Constants.h"

// channel maps
/*
0 - left trackpad
1 - right trackpad
3 - both rumbles
4 - right rumble
5 - everything (or maybe both rumbles?) + restart
6 - both trackpads
7 - nothing
*/


TritonController::TritonController(hid_device *handle) : SteamController(ControllerType::Triton) {
  this->hid_handle = handle;
}

void TritonController::close() {
  hid_close(hid_handle);
}

int TritonController::playNote(int channel, int note, int velocity) {
  byte packet[65] = {0};
  double frequency = midiFrequency[note];
  double period = 1 / frequency;

  if (note == -1) {
    // This prevents the controller from rebooting when using rumble motors and drifting out of tune
    packet[0] = 0x81;
    packet[1] = channel;
  } else {
    packet[0] = 0x83;
    packet[1] = channel;
    packet[2] = velocity;
    packet[3] = (int)frequency % 0xFF;
    packet[4] = (int)frequency / 0xFF;
    packet[5] = 0xFF;
    packet[6] = 0x7F;
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
