#pragma once
#include "SteamController.h"

class TritonController : public SteamController {
  private:
    hid_device *hid_handle;

  public:
    TritonController(hid_device* handle);
    void close() override;
    int playNote(int channel, int note, int velocity) override;
    int sendRaw(byte bytes[], size_t length) override; 
};