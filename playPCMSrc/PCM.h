#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

#ifdef _WIN32
using path_t = std::wstring;
#else
using path_t = std::string;
#endif

class PCM {
private:
  path_t filePath;
  FILE* pipe = nullptr;
  bool ended = false;

  static constexpr int CHUNK_SIZE = 8;
  
  path_t buildCommand() const;
  void start();
  
  public:
  std::uintmax_t fileSize;
  PCM();
  ~PCM();
  
  void stop();

  int load(const path_t& filePath);


  int getNextChunk(unsigned char* buffer);
  // Read up to `size` bytes into buffer. Returns number of bytes read, 0 on EOF, or -1 on error.
  int getBytes(unsigned char* buffer, int size);

  bool eof();
  void reset();
};