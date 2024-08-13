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
  /* ensures output is between 0-1 */
  float out = static_cast<float>(rng_state_) / static_cast<float>(UINT32_MAX);
  return out;
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
  /* convert to samples */
  spawn_pos_ = static_cast<size_t>(knob_val * static_cast<float>(audio_len_-1));
}

void GranularSynth::SetUserActiveGrains(float knob_val){
  // NOTE: set to max 5 grains atm - can change to 20 
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

/* nb: */
void GranularSynth::ApplyRandomness(){
  // /* here we get a random number from 0:1, change the range to -0.5:0.5,
  //     scale it by 2 so range is -1.0:1.0, multiply by randomness for
  //     the param set by knob, then add 1 so range is 1.0f +/- randomness */
  // grain_size_ *= (RngFloat()-0.5f) * 2.0f * rnd_size_ + 1.0f;

  // /* same as above but we add result, multiply by audio len in samples */
  // spawn_pos_ += (RngFloat()-0.5f) * 2.0f * rnd_spawn_pos_ * audio_len_;
  // pitch_ratio_ *= (RngFloat()-0.5f) * 2.0f * rnd_pitch_ + 1.0f;
  // pan_ += (RngFloat()-0.5f) * 2.0f * rnd_pan_;

  if (rnd_size_>0){
    float rnd_size_factor = fmap(RngFloat(),1.0f-rnd_size_, 1.0f+rnd_size_, Mapping::EXP);
    float size_ms = SamplesToMs(grain_size_) * rnd_size_factor;
    size_ms = fclamp(size_ms, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
    grain_size_ = MsToSamples(size_ms);
  }
  
  if (rnd_spawn_pos_>0){
    size_t rnd_offset = static_cast<size_t>(fmap(RngFloat(), -1.0f, 1.0f, Mapping::LINEAR));
    spawn_pos_ += audio_len_ * static_cast<size_t>(rnd_spawn_pos_ * rnd_offset);
    if (spawn_pos_<0){ spawn_pos_ = 0; }
    if (spawn_pos_>=audio_len_) { spawn_pos_ = audio_len_ -1; }
  }

  if (rnd_pan_>0){
    float rnd_pan_offset = fmap(RngFloat(), -rnd_pan_, rnd_pan_, Mapping::LINEAR);
    pan_ = fclamp(pan_+rnd_pan_offset, 0.0f, 1.0f);
  }

  if (rnd_pitch_>0){
    float rnd_pitch_factor = fmap(RngFloat(), 1.0f-rnd_pitch_, 1.0+rnd_pitch_, Mapping::EXP);
    pitch_ratio_ = fclamp((pitch_ratio_*rnd_pitch_factor), MIN_PITCH, MAX_PITCH);
  }

  if (rnd_envelope_>0){
    if (RngFloat() < rnd_envelope_){
      int random_env_type =  static_cast<int>(RngFloat()) * NUM_ENV_TYPES;
      env_type_ = static_cast<Grain::EnvelopeType>(random_env_type);
    }
  }

  if (rnd_phasor_>0){
    if (RngFloat() < rnd_phasor_){
      int random_phasor_mode = static_cast<int>(RngFloat()) * NUM_PHASOR_MODES;
      phasor_mode_ = static_cast<GrainPhasor::Mode>(random_phasor_mode);
    }
  }

  if (rnd_count_>0){
    float rnd_count_factor = fmap(RngFloat(),1.0f-rnd_count_,1.0f+rnd_count_, Mapping::EXP);
    active_count_ *= static_cast<size_t>(rnd_count_factor);
    if (active_count_<MIN_GRAINS) { active_count_ = 1; }
    if (active_count_>MAX_GRAINS) { active_count_ = 20; } 
  }
}




void GranularSynth::TriggerGrain(){
  size_t count = 0;
  for(Grain& grain:grains_){
    if (grain.IsActive()) { count++; }
    if (!grain.IsActive() && count<active_count_){
      ApplyRandomness();
      grain.Trigger(spawn_pos_,grain_size_,pitch_ratio_,pan_);
      count++;
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