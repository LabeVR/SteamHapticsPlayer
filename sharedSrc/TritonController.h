#pragma once
#include "SteamController.h"

// have not tested 0x87 and 0x89. i could be completely wrong about them
#pragma pack(push, 1)

typedef enum
{
  RUMBLE = 0x80,
  PULSE = 0x81,
  COMMAND = 0x82,
  LFO_TONE = 0x83,
  LOG_SWEEP = 0x84,
  HAPTIC_SCRIPT = 0x85,
  // guesses, cannot confirm names directly from SDL or otherwise

  PCM_MODE = 0x86,
  PCM_MONO = 0x87,
  PCM_STEREO = 0x88,
  PCM_MONO_WITH_LENGTH = 0x89,
} TritonReportIDs;

/*Structs taken directly from SDL code*/
typedef struct
{
    uint8_t type;
    uint16_t intensity;
    struct
    {
        uint16_t speed;
        int8_t gain;
    } left, right;
} MsgHapticRumble;

typedef struct
{
    uint8_t side;
    uint16_t on_us;
    uint16_t off_us;
    uint16_t repeat_count;
    uint16_t gain_db; 
} MsgHapticPulse;

typedef struct
{
    uint8_t side;
    uint8_t command;
    int8_t gain_db;
} MsgHapticCommand;

typedef struct
{
    uint8_t side;
    int8_t gain_db;
    uint16_t frequency;
    uint16_t duration_ms;
    uint16_t lfo_freq;
    uint8_t lfo_depth;
} MsgHapticLfoTone;

typedef struct
{
    uint8_t side;
    int8_t gain_db;
    uint16_t duration_ms;
    struct
    {
        uint16_t frequency;
    } start, end;
} MsgHapticLogSweep;

typedef struct
{
    uint8_t side;
    uint8_t script_id;
    int8_t gain_db;
} MsgHapticScript;

/*Structs found via RE*/

typedef struct
{
  // still dont quite know what any of these 3 do, however these are best guesses
  uint8_t operation;
  uint8_t side;
  uint8_t param;
} MsgHapticPCMMode;

typedef struct
{
  uint8_t side;
  char data[62];
} MsgHapticPCMMono;

typedef struct
{
  // <=31
  uint8_t length;

  // signed 8 bit pcm le
  char left[31];
  char right[31];

} MsgHapticPCMStereo;

typedef struct
{
  uint8_t length;
  uint8_t side;
  char data[61];
} MsgHapticPCMMonoWithLength;
#pragma pack(pop)


class TritonController : public SteamController {
  private:
    hid_device *hid_handle;

  public:
    TritonController(hid_device* handle);
    void close() override;
    int playNote(int channel, int note, int velocity) override;
    int playFrequency(int channel, double frequency, int velocity) override;
    int sendPCMMode(MsgHapticPCMMode* packet);
    //s8, 8khz
    int sendPCMStereo(MsgHapticPCMStereo* packet);
    int sendRaw(byte bytes[], size_t length) override;
    void setupPCMStreaming();
};