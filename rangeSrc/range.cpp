#include <cstdio>
#include <thread>
#include "ControllerFinder.h"
// mainly for now to figure out wtf i am doing
// but will eventually use to figure out the range of notes the steam controller can make nicely

int main(int argc, char* argv[]) {
  ControllerFinder finder;
  SteamController* cont = finder.getController();
  TritonController* c;
  if (cont->type == ControllerType::Triton) {
    c = static_cast<TritonController*>(cont);
  }

  int x = 0;
  if (argc >= 2 ) {
    x = std::stoi(argv[1]);
  }

  //while (1) {
    //c->playNote(0, 40, 0xFF);
    //c->playNote(1, 40, 0xFF);
    c->playNote(0xFF, 50, 0xFF);
    //c->playNote(3, 127, 0xFF);

    //c->playNote(0, -1, 0xFF);
    //c->playNote(1, -1, 0xFF);
    //c->playNote(2, -1, 0xFF);
    //c->playNote(3, -1, 0xFF);
 // }
  std::cout << std::cin.get();

  return 0;
}