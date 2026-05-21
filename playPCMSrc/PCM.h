#pragma once
#include <fstream>
#include <string>
#include <filesystem>

class PCM {
private:
std::ifstream file;
static const int CHUNK_SIZE = 8;

public:
  std::uintmax_t fileSize;
  PCM(const std::string& filePath);
  ~PCM();

  int getNextChunk(unsigned char* buffer);
  // Read up to `size` bytes into buffer. Returns number of bytes read, 0 on EOF, or -1 on error.
  int getBytes(unsigned char* buffer, int size);

  bool eof();
  void reset();
};