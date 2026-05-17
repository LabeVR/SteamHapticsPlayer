#include "ControllerFinder.h"

hid_device* ControllerFinder::open_steam_controller_hid(uint16_t pid) {
  struct hid_device_info *devs = hid_enumerate(0x28DE, pid);
  hid_device *handle = NULL;
  for (struct hid_device_info *cur = devs; cur != NULL; cur = cur->next) {
    // printf("%u\n", cur->usage_page);
    if (cur->usage_page == 0xFF00) {
      handle = hid_open_path(cur->path);
      if (handle) break;
    }
  }
  hid_free_enumeration(devs);
  return handle;
}

ControllerFinder::ControllerFinder() {
  // init hidAPI and LIBUSB, i mean technically im only bothering with the controller i have so libusb doesnt matter but mimimimi
  if (int r = hid_init() != 0) {
    std::cerr << "HIDAPI Init Error: " << r << std::endl;
    std::cin.ignore();
    exit(1);
  }

  int r = libusb_init(nullptr);
  if (r < 0) {
    std::cerr << "LIBUSB Init Error " << libusb_error_name(r) << std::endl;
    std::cin.ignore();
    exit(1);
  }
  libusb_set_debug(nullptr, LIBUSB_LOG_LEVEL_NONE);
}

SteamController* ControllerFinder::getController() {
  int interfaceNum;
  ControllerType type;

  libusb_device_handle* dev_handle;
  hid_device* hid_handle;
  // Open Steam Controller device
  if ((dev_handle = libusb_open_device_with_vid_pid(NULL, 0x28DE, 0x1102)) != NULL) { // Wired Steam Controller (2015)
    std::cout << "Found wired Steam Controller (2015)" << std::endl;
    interfaceNum = 2;
    type = ControllerType::Original;
    
  } else if ((dev_handle = libusb_open_device_with_vid_pid(NULL, 0x28DE, 0x1142)) != NULL) { // Steam Controller (2015) dongle //TODO: FIX
    std::cout << "Found Steam Dongle, will attempt to use the first Steam Controller (2015)" << std::endl;
    interfaceNum = 1;
    type = ControllerType::Original;

  } else if ((hid_handle = this->open_steam_controller_hid(0x1302)) != NULL) { // Steam Controller (2026)
    std::cout << "Found wired Steam Controller (2026)" << std::endl;
    type = ControllerType::Triton;

  } else if ((hid_handle = this->open_steam_controller_hid(0x1304)) != NULL) { // Steam Puck
    std::cout << "Found Steam Puck, will attempt to use the first Steam Controller (2026)" << std::endl;
    type = ControllerType::Triton;

  } else if ((dev_handle = libusb_open_device_with_vid_pid(NULL, 0x28DE, 0x1205)) != NULL) { // Steam Deck
    std::cout << "Found Steam Deck" << std::endl;
    interfaceNum = 2;
    type = ControllerType::Jupiter;

  } else {
    std::cout << "No device found" << std::endl;
    type = ControllerType::None;
  }

  // If dev_handle is NULL, it's using HIDAPI so skip this
  if (dev_handle != NULL) {
    // On Linux, automatically detach and reattach kernel module
    libusb_set_auto_detach_kernel_driver(dev_handle, 1);
    // Claim the USB interface
    int r = libusb_claim_interface(dev_handle, interfaceNum);
    if (r < 0) {
      std::cout << "Interface claim Error " << libusb_error_name(r) << std::endl;
      std::cin.ignore();
      libusb_close(dev_handle);
      return nullptr;
    }
  }

  switch (type)
  {
  case ControllerType::None:
    exit(0);
    break;
  case ControllerType::Triton:
    return new TritonController(hid_handle);
    break; 
  default:
    exit(0);
    break;
  }

};