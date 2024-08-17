#pragma once 
#include <stdint.h>
#include <stdlib.h>

struct WavHeader {
  int sample_rate;
  /* total size of audio data in wav file excl file/audio format info */
  uint32_t file_size; 
  int16_t channels;
  int16_t bit_depth;
  size_t total_samples;
};