#pragma once
#include "SteamController.h"
#include "TritonController.h"
#include <iostream>

class ControllerFinder {
  private:
  hid_device* open_steam_controller_hid(uint16_t pid);
  public:
  ControllerFinder();
  ~ControllerFinder() = default;

  SteamController* getController();
  
};