#pragma once
#include "stddef.h"
#include <time.h>

constexpr int SAMPLE_RATE = 48000;
constexpr float SAMPLE_RATE_FLOAT = 48000.f;

constexpr uint16_t MAX_FILES = 32;                      
constexpr uint16_t MAX_FNAME_LEN = 64;

/* 16mb - max size of one stereo channel to be loaded into buffers */
constexpr size_t CHNL_BUF_SIZE_ABS = 16*1024*1024;
/* above is absolute size - each sample needs an int16 (2 bytes) so we do (abs_size)/2 */
constexpr size_t CHNL_BUF_SIZE_SAMPS = 8*1024*1024;

/* this is 60s @ 48kHz * 2 channels 
nb: we can return out into one channel by interleaving the channels */
constexpr size_t RECORD_OUT_BUF_SIZE_SAMPS = 60*48000*2;
/* we need 2 bytes for each sample so we do (buf_samps_size)*2 */
constexpr size_t RECORD_OUT_BUF_SIZE_ABS = 60*48000*2*2;

constexpr int BIT_DEPTH = 16;
/* min/max number of concurrent active grains */
constexpr int MIN_GRAINS = 1;
constexpr int MAX_GRAINS = 20;

constexpr float MIN_GRAIN_SIZE_MS = 100.0f; /* 100ms */
constexpr float MAX_GRAIN_SIZE_MS = 3000.0f; /* 3s */
constexpr float MIN_PITCH = 0.5f;
constexpr float MAX_PITCH = 3.0f;

/* integer clamp as can't use std::clamp */
inline constexpr size_t intclamp(size_t val, size_t min, size_t max){
  if (val < min) val = min;
  else if (val > max) val = max;
  return val;
}

/* converts milliseconds to number of samples */
inline constexpr size_t MsToSamples(float ms){
  return static_cast<size_t>(SAMPLE_RATE_FLOAT * (ms*0.001f));
}

/* converts number of samples to milliseconds */
inline constexpr float SamplesToMs(size_t samples){
  return (static_cast<float>(samples) * 1000.0f) / SAMPLE_RATE_FLOAT; 
}

static constexpr int NUM_ENV_TYPES = 4;
static constexpr int NUM_PHASOR_MODES = 4;
static constexpr int NUM_SYNTH_MODES = 7;
constexpr float PARAM_CHANGE_THRESHOLD = 0.01f;

extern uint32_t rng_state;

inline void SeedRng(){
  rng_state = static_cast<uint32_t>(time(nullptr));
}

inline float RngFloat(){
  /* xorshift32 from https://en.wikipedia.org/wiki/Xorshift 
  for simple fast random floats */
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  /* ensures output is between 0-1 */
  float out = static_cast<float>(rng_state) / static_cast<float>(UINT32_MAX);
  return out;
}




