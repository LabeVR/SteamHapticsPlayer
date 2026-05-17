#pragma once
#include <libusb.h>
#include <hidapi.h>

enum class ControllerType {
	None,
	Original,
	Triton,
	Jupiter,
	Galileo
};

class SteamController
{
protected:
  //libusb_device_handle* dev_handle; for other 3
	//int interfaceNum; also for other 3
  public:
  ControllerType type = ControllerType::None;

  SteamController(ControllerType type) {
    this->type = type;
  };
  virtual ~SteamController() = default;
  virtual void close() = 0;
  virtual int playNote(int channel, int note, int velocity) = 0;
  virtual int sendRaw(byte bytes[], size_t length) = 0;
  
};