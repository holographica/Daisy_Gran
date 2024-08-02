#include "GranularSynth.h"

using namespace daisy;

void GranularSynth::Init(int16_t *left, int16_t *right, size_t file_len_samples){
  left_channel_ = left;
  right_channel_ = right;
  file_len_samples_ = file_len_samples;
  len_samples_float = static_cast<float>(file_len_samples);
  // set all default values
  /* get initial knob positions and assign to size and spawn pos params */
  float k1v = MapKnobDeadzone(pod_->knob1.GetRawFloat());
  float k2v = MapKnobDeadzone(pod_->knob2.GetRawFloat());
  SetSize(k1v);
  SetSpawnPos(k2v);
  // set to default vals
  SetPan(0.5f);
  SetPitch(1.0f);
  // TODO: spray, pitch, etc 
  for (Grain& grain:grains_){
    grain.Init(size_, pan_, GrainPhasor::Mode::OneShot);
  }
  grains_[0].ActivateGrain();
  // seed rng with a random number 
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

/* knobs on the Pod have a small deadzone around the upper/lower bounds
  (eg my knob1 only goes down to 0.003) -> assume knob is at 0 or 1 if it's very close */
float GranularSynth::MapKnobDeadzone(float knob_val){
  if (knob_val<=0.003f) { knob_val = 0.0f; }
  else if (knob_val>=0.997f) { knob_val = 1.0f; }
  return knob_val;
}

/* max density (per second) depends on (max no of grains / size in ms)*1000.0f
  eg if size is 10ms, max grains is 20 -> 2000 grains/second
  if size is 500ms -> max is 40 grains/second, etc
  we multiply by 1000 to convert ms -> seconds */
void GranularSynth::UpdateMaxDensity(){
  max_density_ = (MAX_GRAINS * 1000.0f ) / size_;
}

void GranularSynth::SetSize(float size_ms){
  size_ = fclamp(size_ms, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
  /* update max density here as it depends on curr grain size */
  UpdateMaxDensity();
}

void GranularSynth::SetSpawnPos(float pos){
  spawn_pos_ = fclamp(pos, 0.0f, 1.0f);
}

void GranularSynth::SetDensity(float density){
  density_ = fclamp(density, MIN_DENSITY, max_density_);
}

void GranularSynth::SetPan(float pan){
  pan_ = fclamp(pan, 0.0f, 1.0f);
}

void GranularSynth::SetPitch(float ratio){
  // clamp between 0.25x speed and 4x speed 
  pitch_ratio_ = fclamp(ratio,0.25f,4.0f);
}

void GranularSynth::SetPhasorMode(GrainPhasor::Mode mode){
  curr_phasor_mode_ = mode;
}

int GranularSynth::GetAvailableGrainIdx() const{
  for (int i=0;i<MAX_GRAINS;i++){
    if (!grains_[i].IsActive()){
      return i;
    }
  }
  return -1;
}

void GranularSynth::TriggerGrain(){
  int idx = GetAvailableGrainIdx();
  // return if no inactive grains available
  if (idx == -1) return;
  float centre_spawn_pos = spawn_pos_ * len_samples_float;
  float max_spray = spray_ * len_samples_float;
  /* range of spray param is (-1:+1) but range of random float is (0:1)
  so multiply by 2 so range (0:2), then subtract 1 so we get range (-1:+1) */
  float rand_spray = (RngFloat() * 2.0f) - 1.0f;
  float spray_offset = rand_spray* max_spray;
  /* add offset, then clamp between 0 and end of audio sample */
  float grain_spawn_pos = fclamp(centre_spawn_pos + spray_offset,0.0f,(len_samples_float-1));
  grains_[idx].Trigger(size_, grain_spawn_pos, pitch_ratio_,curr_phasor_mode_);
}

void GranularSynth::ProcessGrains(float *out_left, float *out_right, size_t size){
  /* find how many samples between triggering each grain */
  float samps_per_grain = SAMPLE_RATE_FLOAT / density_;
  for (size_t i=0; i<size; i++){
    since_last_grain_ +=1.0f;
    if (since_last_grain_ >= samps_per_grain){
      TriggerGrain();
      since_last_grain_ -= samps_per_grain;
    }
    float sum_left = 0.0f;
    float sum_right = 0.0f;
    size_t active_grains =0;
    for (Grain& grain:grains_){
      if (grain.IsActive()){
        Sample samp = grain.Process(left_channel_, right_channel_);
        sum_left += samp.left;
        sum_right += samp.right;
        active_grains++;
      }
    }
    /* average output of grains so we don't clip */
    if (active_grains>0){
      out_left[i] = sum_left/active_grains;
      out_right[i] = sum_right/active_grains;
    }
    else { out_left[i] = out_right[i] = 0.0f; }
  }
}

// TODO:
// implement menu states so i can change diff params
// map parameter values to knob values
// make sure param changes are reflected to grains 




