#include "GranularSynth.h"

using namespace daisy;

/// @brief Initialise granular synth object and assign audio buffers
/// @param left Left channel audio data buffer
/// @param right Right channel audio data buffer
/// @param audio_len Length of currently loaded audio file in samples
void GranularSynth::Init(int16_t *left, int16_t *right, size_t audio_len){
  left_buf_ = left;
  right_buf_ = right;
  audio_len_ = audio_len;
  for (Grain& grain: grains_){
    grain.Init();
  }
  Grain::audio_len_ = audio_len;
  Grain::left_buf_ = left;
  Grain::right_buf_ = right;
  InitParams();
  rng_->Init(SAMPLE_RATE_FLOAT);
  rng_->SetFreq(100.0f);
  rnd_bias_ = 0.0f;
  rnd_amount_ = 0.4f;
  // DebugPrint(pod_, "rnd %f", rng_->Process());
  // DebugPrint(pod_, "rnd %f", rng_->Process());
  // DebugPrint(pod_, "rnd %f", rng_->Process());
  // DebugPrint(pod_, "rnd %f", rng_->Process());
  rnd_bias_=0.0f;
  rnd_amount_=0.0f;
}

void GranularSynth::Reset(size_t len){
  audio_len_ = len;
  for (Grain& grain: grains_){
    grain.Init();
  }
  InitParams();
}

/// @brief  Set intial grain parameter values
void GranularSynth::InitParams(){
  grain_size_ = 4800;
  spawn_pos_ = 0;
  curr_active_count_ = 1;
  pitch_ratio_ = 1.0f;
  pan_ = 0.5f;
  direction_ = 1.0f;
}

void GranularSynth::SetRndAmount(float amount){
  rnd_amount_ = fclamp(amount, 0.01f, 1.0f);
}

void GranularSynth::SetRndBias(float bias){
  rnd_bias_ = fclamp(bias, 0.01f, 1.0f);
}

// void GranularSynth::SetRndRefreshFreq(float freq){
//   /* map knob input between 0.1Hz and 20Hz on an exponential curve */
//   freq = fmap(freq, 0.1f, 20.0f, daisysp::Mapping::EXP);
//   rng_->SetFreq(freq);
// }

/* get random number then map it to bias
    - at low bias, range is clustered around random number generated
    - at mid bias, range is more of a uniform distribution
    - at high bias,  */
float GranularSynth::GetRnd(){
  float rnd = rng_->Process();
  return rnd*rnd_amount_;
  /* find the bias range for random number distribution  */
  // float bias = fmap(rnd_bias_, rnd, -rnd, daisysp::Mapping::EXP);
  // return (bias * rnd_amount_);
}

/* these setters take a normalised value (ie float from 0-1) 
  from the user input knobs and convert this to the correct units */

void GranularSynth::SetGrainSize(float knob_val){
    /* add some subtle randomness and clamp to knob value bounds */
  float randomness = GetRnd();
  knob_val = fclamp(knob_val+randomness, 0.0f, 1.0f);
  float size_ms = fmap(knob_val, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
  grain_size_ = MsToSamples(size_ms);
}

void GranularSynth::SetSpawnPos(float knob_val){
  /* add some subtle randomness and clamp to knob value bounds */
  float randomness = GetRnd();
  knob_val = fclamp(knob_val+randomness, 0.0f, 1.0f);
  /* convert to samples */
  spawn_pos_ = static_cast<size_t>(knob_val * static_cast<float>(audio_len_-1));
}

void GranularSynth::SetTargetActiveGrains(float knob_val){
  // float randomness = GetRnd();
  // float count = round(fmap(knob_val+randomness, MIN_GRAINS, MAX_GRAINS));
  knob_val = round(fmap(knob_val, MIN_GRAINS, MAX_GRAINS));
  target_active_count_ = (static_cast<size_t>(knob_val));
}

/* this increases the number of active grains more smoothly - had dropout issues without */
void GranularSynth::UpdateActiveGrains(){
  curr_active_count_ = (curr_active_count_ * GRAIN_INCREASE_SMOOTHNESS)
                        + (target_active_count_ * (1.0f-GRAIN_INCREASE_SMOOTHNESS));
}

size_t GranularSynth::GetActiveGrains(){
  return static_cast<size_t>(curr_active_count_);
}

void GranularSynth::SetPitchRatio(float ratio){
  /* add a small amount of randomness and clamp to knob value bounds */
  float randomness = GetRnd()*0.1f;
  ratio = fclamp(ratio+randomness, 0.0f, 1.0f);
  pitch_ratio_ = fmap(ratio, 0.5, 2, daisysp::Mapping::LINEAR);
}

// void GranularSynth::SetActiveGrains(float count){
//   curr_active_count_ = static_cast<size_t>(round(fmap(count, 1.0f, 20.0f)));
// }


/* we don't add randomness to pan since we have the stereo rotator FX */
void GranularSynth::SetPan(float pan){
  pan_ = pan;
}
/* don't add randomness as it wouldn't sound great */
void GranularSynth::SetDirection(float direction){
  if (direction >0.5) direction_ = -1.0f;
  else direction = 1.0f;
}

/// @brief Update grain audio parameters
void GranularSynth::UpdateGrainParams(){
  for (Grain& grain:grains_){
    grain.SetGrainSize(grain_size_);
    grain.SetSpawnPos(spawn_pos_);
    grain.SetPitchRatio(pitch_ratio_);
    grain.SetDirection(direction_);
  }
}

/// @brief Triggers new grains and applies randomness to grain parameters
void GranularSynth::TriggerGrain(){
  size_t count = 0;
  for(Grain& grain:grains_){
    // UpdateGrainParams();
    if (grain.is_active_) { count++; }
    else{
    // else if (count<GetActiveGrains()){
      grain.Trigger(spawn_pos_,grain_size_,pitch_ratio_,pan_, direction_);
      // count++;
      break;
    }
  }
}

/// @brief Processes and sums audio of active grains, then mixes to output buffers
/// @param out_left Pointer to left channel audio output buffer
/// @param out_right Pointer to right channel audio output buffer
/// @param size Number of samples to process in this call 
Sample GranularSynth::ProcessGrains(){
  // UpdateActiveGrains();
  sample_.left=0.0f, sample_.right=0.0f;
  TriggerGrain();
  for (Grain& grain:grains_){
    if (grain.is_active_){
      sample_ = grain.Process(sample_);
      // if (count%60000==0){
      //   // DebugPrint(pod_, "lbuf %f, rbuf %f, envv %f",grains_[0].lbuf, grains_[0].rbuf, grains_[0].envv);
      //   DebugPrint(pod_, "lbuf %f, rbuf%f env %f",grain.lbuf, grain.rbuf, grain.envv);
      //   // DebugPrint(pod_, "activecount %u pan %f",active_count_,pan_); 
      // }
    }
  }
  // count++;
  // if (count%60000==0){
  //   count=0;
  // //   // DebugPrint(pod_, "lbuf %f, rbuf %f, envv %f",grains_[0].lbuf, grains_[0].rbuf, grains_[0].envv);
  //   DebugPrint(pod_, "sampL %f, sampR%f", sample_.left, sample_.right);
  //   DebugPrint(pod_, "activecount %u pan %f",GetActiveGrains(),pan_); 
  // }
  return sample_;
}


// NOTE //TODO
// NOTE //TODO
// TODO //NOTE
// TODO //NOTE
/*
have 2 parameters for randomness
- one is fixed eg 0.05*random number added to every parameter 
    so there is always some degree of randomness

- one is controlled by randomness parameter

NEED TO FIX REVERB/FX SINCE NOT WORKING PROPERLY ATM - WHY?



*/



