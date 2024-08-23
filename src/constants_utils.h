#pragma once
#include "stddef.h"
#include <time.h>

/* audio constants */
constexpr int SAMPLE_RATE = 48000;
constexpr float SAMPLE_RATE_FLOAT = 48000.f;
constexpr int BIT_DEPTH = 16;

/* delay line buffer size (2s @ 48kHz) */
constexpr size_t DELAY_TIME = 96000;

/* maximum length of recording to SD card */
constexpr size_t MAX_REC_OUT_LEN = 120;

/* 16mb - max size of one stereo channel to be loaded into buffers */
constexpr size_t CHNL_BUF_SIZE_ABS = 16*1024*1024;
/* above is absolute size - each sample needs an int16 (2 bytes) so we do (abs_size)/2 */
constexpr size_t CHNL_BUF_SIZE_SAMPS = 8*1024*1024;

/* chunk size for reading audio into temporary buffer */
const size_t BUF_CHUNK_SZ = 16384;

/* file reading constants*/
static const uint16_t MAX_FILES = 32;                      
static const uint16_t MAX_FNAME_LEN = 128;

/* granular synth parameter constants */
constexpr int MIN_GRAINS = 1;
constexpr int MAX_GRAINS = 10;
constexpr float GRAIN_INCREASE_SMOOTHNESS = 0.99f;
static constexpr int NUM_SYNTH_MODES = 8;
constexpr float PARAM_CHANGE_THRESHOLD = 0.01f;
constexpr float MIN_GRAIN_SIZE_MS = 100.0f;
constexpr float MAX_GRAIN_SIZE_MS = 3000.0f;
constexpr size_t MIN_GRAIN_SIZE_SAMPLES = 4800; /* 100ms at 48kHz */
constexpr size_t MAX_GRAIN_SIZE_SAMPLES = 144000; /* 3s at 48kHz */
constexpr float MIN_PITCH = 0.5f;
constexpr float MAX_PITCH = 3.0f;


/* button event constants */
const unsigned long DEBOUNCE_DELAY = 50; 
const unsigned long LONG_PRESS_TIME = 1000;

/* hipass filter frequency range */
const float HIPASS_LOWER_BOUND = 0.0004f;
const float HIPASS_UPPER_BOUND = 0.01f;

/* lowpass filter frequency range */
const float LOPASS_LOWER_BOUND = 20.0f;
const float LOPASS_UPPER_BOUND = 18000.0f;

// const float HICUT_FREQ = 0.3125f; /* 15000Hz @ 48kHz sample rate */
const float HICUT_FREQ = 0.34375; /* 16500Hz @ 48kHz sample rate */

/* random number generator variables and helper functions */
extern uint32_t rng_state;
static inline void SeedRng(){
  rng_state = static_cast<uint32_t>(time(nullptr));
}
static inline float RngFloat(){
  /* xorshift32 from https://en.wikipedia.org/wiki/Xorshift 
  for simple fast random floats */
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  /* ensures output is between 0-1 */
  float out = static_cast<float>(rng_state) / static_cast<float>(UINT32_MAX);
  return out;
}

/* integer clamp as can't use std::clamp */
static inline constexpr size_t intclamp(size_t val, size_t min, size_t max){
  if (val < min) val = min;
  else if (val > max) val = max;
  return val;
}

/* converts milliseconds to number of samples */
static inline constexpr size_t MsToSamples(float ms){
  return static_cast<size_t>(SAMPLE_RATE_FLOAT * (ms*0.001f));
}

/* converts number of samples to milliseconds */
static inline constexpr float SamplesToMs(size_t samples){
  return (static_cast<float>(samples) * 1000.0f) / SAMPLE_RATE_FLOAT; 
}

/* fast cos and sin approximations
  thanks and credit to Jack Ganssle: 
  https://www.ganssle.com/item/approximations-for-trig-c-code.htm
  https://www.ganssle.com/approx/sincos.cpp
*/
constexpr float halfpi = M_PI/2.0f;

static inline constexpr float FastCos(float x){
  const float c1= 0.99940307;
  const float c2=-0.49558072;
  const float c3= 0.03679168;

  float x2 = x * x;
  return (c1 + x2*(c2 + c3 * x2));
}


static inline constexpr float FastSin(float x){
	return FastCos(halfpi-x);
}

/* keep angle of rotation within bounds +-pi */
static inline constexpr float NormaliseRotationAngle(float rotation){
  while (rotation > M_PI) rotation -= 2*M_PI;
  while (rotation < -M_PI) rotation += 2*M_PI;
  return rotation;
}