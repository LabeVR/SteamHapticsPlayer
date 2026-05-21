#pragma once
#include <fstream>
#include <string>

class PCM {
private:
  std::ifstream file;
  static constexpr int CHUNK_SIZE = 8;

public:
  PCM(const std::string& filePath);
  ~PCM();

  int getNextChunk(unsigned char* buffer);
  // Read up to `size` bytes into buffer. Returns number of bytes read, 0 on EOF, or -1 on error.
  int getBytes(unsigned char* buffer, int size);
  bool eof();
  void reset();
};