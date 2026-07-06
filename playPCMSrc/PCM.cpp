#include "PCM.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>
#include <windows.h>
#endif

#ifdef _WIN32
#define POPEN wpopen
#define PCLOSE pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

bool ffmpegAvailable() {
#ifdef _WIN32
  return std::system("ffmpeg -version >nul 2>nul") == 0;
#else
  return std::system("ffmpeg -version >/dev/null 2>&1") == 0;
#endif
}

PCM::PCM() : filePath(), pipe(nullptr), ended(false), liveSource(false), fileSize(0) {}

PCM::~PCM() {
  closePipe();
}

int PCM::load(const path_t& filePath, bool liveSource) {
  this->filePath = filePath;
  this->liveSource = liveSource;

  if (!this->liveSource && this->filePath.empty()) {
    return -1;
  }

  if (!ffmpegAvailable()) {
    return -2;
  }

#ifndef _WIN32
  if (this->liveSource) {
    return -3;
  }
#endif

  try {
    start();
  } catch (...) {
    return -1;
  }

  return 0;
}

path_t PCM::buildCommand() const {
#ifdef _WIN32
  std::wostringstream cmd;
  cmd << L"ffmpeg -hide_banner -loglevel error ";
  cmd << L"-i \"" << filePath << L"\" ";
  cmd << L"-f s8 -ac 2 -ar 8000 -acodec pcm_s8 pipe:1";
  return cmd.str();
#else
  std::ostringstream cmd;
  cmd << "ffmpeg -hide_banner -loglevel error ";
  cmd << "-i \"" << filePath << "\" ";
  cmd << "-f s8 -ac 2 -ar 8000 -acodec pcm_s8 pipe:1";
  return cmd.str();
#endif
}

void PCM::closePipe() {
#ifdef _WIN32
  if (liveSource) {
    stopLiveCapture();
    return;
  }
#endif
  if (pipe) {
    PCLOSE(pipe);
    pipe = nullptr;
  }
}

void PCM::start() {
  if (!liveSource && filePath.empty()) {
    throw std::runtime_error("No input file loaded");
  }

#ifdef _WIN32
  if (liveSource) {
    startLiveCapture();
    ended = false;
    fileSize = 0;
    return;
  }
#endif

#ifdef _WIN32
  std::wstring command = buildCommand();
  pipe = POPEN(command.c_str(), L"rb");
#else
  std::string command = buildCommand();
  pipe = POPEN(command.c_str(), "r");
#endif
  if (!pipe) {
    std::cerr << "popen failed, errno=" << errno << " (" << strerror(errno) << ")\n";
    throw std::runtime_error("Could not start ffmpeg");
  }

  ended = false;
  fileSize = 0;
  pcmBytes.clear();
  readPointer = 0;

  if (liveSource) {
    return;
  }

  while (!ended) {
    uint8_t buff[64];
    std::size_t bytesRead = std::fread(buff, 1, sizeof(buff), pipe);
    if (bytesRead == 0) {
      if (std::feof(pipe) || std::ferror(pipe)) {
        ended = true;
        break;
      }
      continue;
    }
    pcmBytes.insert(pcmBytes.end(), buff, buff + bytesRead);
  }

  closePipe();
  fileSize = pcmBytes.size();
  ended = false;
}

int PCM::getNextChunk(uint8_t* buffer) {
  return getBytes(buffer, CHUNK_SIZE);
}

int PCM::getBytes(uint8_t* buffer, int size) {
  if (size <= 0 || buffer == nullptr) return -1;

#ifdef _WIN32
  if (liveSource) {
    std::unique_lock<std::mutex> lock(liveMutex);
    liveCv.wait(lock, [&] {
      return static_cast<int>(liveBytes.size()) >= size || (!liveCaptureRunning && !liveBytes.empty()) || (!liveCaptureRunning && liveBytes.empty());
    });

    if (liveBytes.empty() && !liveCaptureRunning) {
      ended = true;
      return 0;
    }

    int toCopy = static_cast<int>(std::min<std::size_t>(liveBytes.size(), static_cast<std::size_t>(size)));
    for (int i = 0; i < toCopy; ++i) {
      buffer[i] = liveBytes.front();
      liveBytes.pop_front();
    }
    return toCopy;
  }
#endif

  if (liveSource) {
    if (!pipe) {
      ended = true;
      return 0;
    }

    size_t totalRead = 0;
    while (totalRead < static_cast<size_t>(size)) {
      std::size_t bytesRead = std::fread(buffer + totalRead, 1, static_cast<size_t>(size) - totalRead, pipe);
      if (bytesRead == 0) {
        if (std::feof(pipe) || std::ferror(pipe)) {
          ended = true;
          break;
        }
        continue;
      }
      totalRead += bytesRead;
    }

    return static_cast<int>(totalRead);
  }

  if (readPointer >= pcmBytes.size()) {
    ended = true;
    return 0;
  }

  size_t avail = pcmBytes.size() - readPointer;
  size_t toCopy = static_cast<size_t>(size);
  if (avail < toCopy) toCopy = avail;

  std::memcpy(buffer, pcmBytes.data() + readPointer, toCopy);
  readPointer += toCopy;
  if (readPointer >= pcmBytes.size()) ended = true;
  return static_cast<int>(toCopy);
}

void PCM::reset() {
  closePipe();
  pcmBytes.clear();
  readPointer = 0;
  start();
}

#ifdef _WIN32
namespace {
constexpr GUID PCM_SUBTYPE_GUID{0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
constexpr GUID IEEE_FLOAT_SUBTYPE_GUID{0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
constexpr std::size_t MAX_LIVE_BUFFER_BYTES = 256;

double sampleToDouble(const BYTE* frame, const WAVEFORMATEX* format, UINT16 channelIndex) {
  if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
    const auto* samples = reinterpret_cast<const float*>(frame);
    return samples[channelIndex];
  }

  if (format->wFormatTag == WAVE_FORMAT_PCM) {
    if (format->wBitsPerSample == 16) {
      const auto* samples = reinterpret_cast<const std::int16_t*>(frame);
      return samples[channelIndex] / 32768.0;
    }
    if (format->wBitsPerSample == 32) {
      const auto* samples = reinterpret_cast<const std::int32_t*>(frame);
      return samples[channelIndex] / 2147483648.0;
    }
    if (format->wBitsPerSample == 8) {
      return (static_cast<int>(frame[channelIndex]) - 128) / 128.0;
    }
  }

  if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
    if (IsEqualGUID(ext->SubFormat, IEEE_FLOAT_SUBTYPE_GUID)) {
      const auto* samples = reinterpret_cast<const float*>(frame);
      return samples[channelIndex];
    }
    if (IsEqualGUID(ext->SubFormat, PCM_SUBTYPE_GUID)) {
      if (format->wBitsPerSample == 16) {
        const auto* samples = reinterpret_cast<const std::int16_t*>(frame);
        return samples[channelIndex] / 32768.0;
      }
      if (format->wBitsPerSample == 32) {
        const auto* samples = reinterpret_cast<const std::int32_t*>(frame);
        return samples[channelIndex] / 2147483648.0;
      }
    }
  }

  return 0.0;
}

uint8_t toS8(double value) {
  value = std::clamp(value, -1.0, 1.0);
  int sample = static_cast<int>(std::lround(value * 127.0));
  if (sample < -128) sample = -128;
  if (sample > 127) sample = 127;
  return static_cast<uint8_t>(static_cast<std::int8_t>(sample));
}
} // namespace

void PCM::pushLiveBytes(const uint8_t* data, size_t size) {
  if (data == nullptr || size == 0) return;
  {
    std::lock_guard<std::mutex> lock(liveMutex);
    liveBytes.insert(liveBytes.end(), data, data + size);
    while (liveBytes.size() > MAX_LIVE_BUFFER_BYTES) {
      liveBytes.pop_front();
    }
  }
  liveCv.notify_all();
}

void PCM::stopLiveCapture() {
  {
    std::lock_guard<std::mutex> lock(liveMutex);
    liveCaptureStopRequested = true;
  }
  liveCv.notify_all();
  if (liveCaptureThread.joinable()) {
    liveCaptureThread.join();
  }
  {
    std::lock_guard<std::mutex> lock(liveMutex);
    liveCaptureRunning = false;
    liveCaptureStopRequested = false;
    liveBytes.clear();
  }
}

void PCM::startLiveCapture() {
  stopLiveCapture();
  {
    std::lock_guard<std::mutex> lock(liveMutex);
    liveBytes.clear();
    liveCaptureStopRequested = false;
    liveCaptureRunning = true;
  }
  liveCaptureThread = std::thread(&PCM::liveCaptureLoop, this);
}

void PCM::liveCaptureLoop() {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  bool comInitialized = SUCCEEDED(hr);

  IMMDeviceEnumerator* enumerator = nullptr;
  IMMDevice* device = nullptr;
  IAudioClient* audioClient = nullptr;
  IAudioCaptureClient* captureClient = nullptr;
  WAVEFORMATEX* mixFormat = nullptr;
  HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  auto finish = [&](bool keepRunning) {
    if (captureClient) captureClient->Release();
    if (audioClient) {
      audioClient->Stop();
      audioClient->Release();
    }
    if (device) device->Release();
    if (enumerator) enumerator->Release();
    if (mixFormat) CoTaskMemFree(mixFormat);
    if (eventHandle) CloseHandle(eventHandle);
    if (comInitialized) CoUninitialize();

    {
      std::lock_guard<std::mutex> lock(liveMutex);
      liveCaptureRunning = keepRunning;
      liveCaptureStopRequested = false;
    }
    liveCv.notify_all();
  };

  if (FAILED(hr)) {
    finish(false);
    return;
  }

  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
  if (FAILED(hr) || enumerator == nullptr) {
    finish(false);
    return;
  }

  hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
  if (FAILED(hr) || device == nullptr) {
    finish(false);
    return;
  }

  hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient));
  if (FAILED(hr) || audioClient == nullptr) {
    finish(false);
    return;
  }

  hr = audioClient->GetMixFormat(&mixFormat);
  if (FAILED(hr) || mixFormat == nullptr) {
    finish(false);
    return;
  }

  const UINT32 inputSampleRate = mixFormat->nSamplesPerSec;
  const UINT16 inputChannels = mixFormat->nChannels;
  const UINT16 inputBlockAlign = mixFormat->nBlockAlign;
  const double outputRate = 8000.0;

  REFERENCE_TIME bufferDuration = 1000000;
  hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, bufferDuration, 0, mixFormat, nullptr);
  if (FAILED(hr)) {
    finish(false);
    return;
  }

  hr = audioClient->SetEventHandle(eventHandle);
  if (FAILED(hr)) {
    finish(false);
    return;
  }

  hr = audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureClient));
  if (FAILED(hr) || captureClient == nullptr) {
    finish(false);
    return;
  }

  hr = audioClient->Start();
  if (FAILED(hr)) {
    finish(false);
    return;
  }

  double resampleAccumulator = 0.0;
  std::vector<uint8_t> produced;
  produced.reserve(256);

  while (true) {
    {
      std::lock_guard<std::mutex> lock(liveMutex);
      if (liveCaptureStopRequested) break;
    }

    DWORD waitResult = WaitForSingleObject(eventHandle, 100);
    if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT) {
      break;
    }

    for (;;) {
      UINT32 packetLength = 0;
      hr = captureClient->GetNextPacketSize(&packetLength);
      if (FAILED(hr) || packetLength == 0) {
        break;
      }

      BYTE* data = nullptr;
      UINT32 numFrames = 0;
      DWORD flags = 0;
      hr = captureClient->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr);
      if (FAILED(hr) || data == nullptr) {
        break;
      }

      if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0) {
        std::vector<uint8_t> silence(numFrames * 2, 0);
        pushLiveBytes(silence.data(), silence.size());
        captureClient->ReleaseBuffer(numFrames);
        continue;
      }

      produced.clear();
      for (UINT32 frameIndex = 0; frameIndex < numFrames; ++frameIndex) {
        resampleAccumulator += outputRate;
        if (resampleAccumulator < static_cast<double>(inputSampleRate)) {
          continue;
        }
        resampleAccumulator -= static_cast<double>(inputSampleRate);

        const BYTE* frame = data + (frameIndex * inputBlockAlign);
        double left = sampleToDouble(frame, mixFormat, 0);
        double right = inputChannels > 1 ? sampleToDouble(frame, mixFormat, 1) : left;

        produced.push_back(toS8(left));
        produced.push_back(toS8(right));
      }

      if (!produced.empty()) {
        pushLiveBytes(produced.data(), produced.size());
      }

      hr = captureClient->ReleaseBuffer(numFrames);
      if (FAILED(hr)) {
        break;
      }
    }
  }

  finish(false);
}
#endif