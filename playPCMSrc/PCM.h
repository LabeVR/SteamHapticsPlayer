#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

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
  bool liveSource = false;
  std::vector<uint8_t> pcmBytes = std::vector<uint8_t>();
  size_t readPointer = 0;

#ifdef _WIN32
  std::thread liveCaptureThread;
  std::mutex liveMutex;
  std::condition_variable liveCv;
  std::deque<uint8_t> liveBytes;
  bool liveCaptureRunning = false;
  bool liveCaptureStopRequested = false;
#endif

  static constexpr int CHUNK_SIZE = 8;
  
  path_t buildCommand() const;
  void closePipe();
  void start();
#ifdef _WIN32
  void startLiveCapture();
  void stopLiveCapture();
  void liveCaptureLoop();
  void pushLiveBytes(const uint8_t* data, size_t size);
#endif
  
  public:
  std::uintmax_t fileSize;
  PCM();
  ~PCM();
  
  int load(const path_t& filePath, bool liveSource = false);

  int getNextChunk(uint8_t* buffer);
  // Read up to `size` bytes into buffer. Returns number of bytes read, 0 on EOF, -1 on error
  int getBytes(uint8_t* buffer, int size);

  void reset();
};