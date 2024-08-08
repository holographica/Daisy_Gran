#include "GranularSynth.h"

using namespace daisy;

void GranularSynth::Init(const int16_t *left, const int16_t *right, size_t audio_len){
  left_buf_ = left;
  right_buf_ = right;
  audio_len_ = audio_len;
  for (Grain& grain: grains_){
    grain.Init(left,right,audio_len,&pod_);
  }
  SeedRng(static_cast<uint32_t>(rand()));
}

void GranularSynth::SeedRng(uint32_t seed){
  rng_state_ = seed;
}

float GranularSynth::RngFloat(){
  /* xorshift32 from https://en.wikipedia.org/wiki/Xorshift 
  for simple fast random floats */
  rng_state_ ^= rng_state_ << 13;
  rng_state_ ^= rng_state_ >> 17;
  rng_state_ ^= rng_state_ << 5;
  /* ensures output is between 0 and 1 */
  float out = static_cast<float>(rng_state_) / static_cast<float>(UINT32_MAX);
  return out;
}

/* don't need a separate user setter for this
  since pan range is already 0-1 */
void GranularSynth::SetPan(float pan){
  pan_ = pan;
}

void GranularSynth::SetPhasorMode(GrainPhasor::Mode mode){
  phasor_mode_ = mode;
}


/* these setters are used by the program
  in setup etc rather than controlled by user input */

void GranularSynth::SetGrainSize(float size_ms){
  size_ms = fclamp(size_ms, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
  grain_size_ = MsToSamples(size_ms);
}

void GranularSynth::SetSpawnPosSamples(size_t pos){
  spawn_pos_ = std::min(pos, audio_len_-1);
}

void GranularSynth::SetActiveGrains(size_t count){
  if (count<0) { count=0; }
  if (count>20) { count=20; }
  active_count_ = count;
}

void GranularSynth::SetPitchRatio(float ratio){
  pitch_ratio_ = ratio;
}


/* these setters take a normalised value (ie float from 0-1) 
  from the user input knobs and convert this to the correct units */

void GranularSynth::SetUserGrainSize(float knob_val){
  float size_ms = fmap(knob_val, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
  SetGrainSize(size_ms);
}

void GranularSynth::SetUserSpawnPos(float knob_val){
  // float npos = fclamp(knob_val, 0.0f, 1.0f);
  /* convert to samples */
  spawn_pos_ = static_cast<size_t>(knob_val * static_cast<float>(audio_len_-1));
}

void GranularSynth::SetUserActiveGrains(float knob_val){
  float count = round(fmap(knob_val, 1.0f, 5.0f));
  SetActiveGrains(static_cast<size_t>(count));
}

void GranularSynth::SetUserPitchRatio(float ratio){
  float pitch = fmap(ratio, 0.5, 2, daisysp::Mapping::LOG);
  SetPitchRatio(pitch);
}

void GranularSynth::UpdateGrainParams(){
  for (Grain& grain:grains_){
    grain.SetGrainSize(grain_size_);
    grain.SetSpawnPos(spawn_pos_);
    grain.SetPitchRatio(pitch_ratio_);
  }
}

void GranularSynth::TriggerGrain(){
  size_t count = 0;
  for(Grain& grain:grains_){
    if (grain.IsActive()) { count++; }
    if (!grain.IsActive() && count<active_count_){
      grain.Trigger(spawn_pos_,grain_size_,pitch_ratio_,pan_);
      break;
    }
  }
}


void GranularSynth::ProcessGrains(float *out_left, float *out_right, size_t size){
  for (size_t i=0; i<size;i++){
    TriggerGrain();
    float sum_left = 0.0f;
    float sum_right = 0.0f;
    size_t active = 0;
    for (Grain& grain:grains_){
      if (grain.IsActive()){
        grain.Process(&sum_left,&sum_right);
        active++;
      }
    }
    if (active>0){
      out_left[i]=(sum_left/active);
      out_right[i]=(sum_right/active);
    } 
    else {
      out_left[i]=0.0f;
      out_right[i]=0.0f;
    }
  }
}