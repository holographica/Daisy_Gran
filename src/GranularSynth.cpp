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
  active_count_ = 1;
  pitch_ratio_ = 1.0f;
  pan_ = 0.5f;
  direction_ = 0.0f;
}

/* these setters take a normalised value (ie float from 0-1) 
  from the user input knobs and convert this to the correct units */

void GranularSynth::SetGrainSize(float knob_val){
  float size_ms = fmap(knob_val, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
  grain_size_ = MsToSamples(size_ms);
}

void GranularSynth::SetSpawnPos(float knob_val){
  /* convert to samples */
  spawn_pos_ = static_cast<size_t>(knob_val * static_cast<float>(audio_len_-1));
}

void GranularSynth::SetActiveGrains(float knob_val){
  float count = round(fmap(knob_val, 1.0f, 20.0f));
  active_count_ = (static_cast<size_t>(count));
}

void GranularSynth::SetPitchRatio(float ratio){
  pitch_ratio_ = fmap(ratio, 0.5, 2, daisysp::Mapping::LINEAR);
}

void GranularSynth::SetPan(float pan){ 
  pan_ = pan;
}

void GranularSynth::SetDirection(float direction){
  direction_ = direction;
}

/// @brief Update grain audio parameters
void GranularSynth::UpdateGrainParams(){
  for (Grain& grain:grains_){
    grain.SetGrainSize(grain_size_);
    grain.SetSpawnPos(spawn_pos_);
    grain.SetPitchRatio(pitch_ratio_);
    grain.SetPhasorDirection(direction_);
  }
}


/// @brief We take a random number, map it into a certain range
///        determined by the user randomness setting for that parameter,
///        then clamp it so it stays in the correct range for the parameter 
void GranularSynth::ApplyRandomness(){
  /* here we map a randomly generated num between 1 +/- user randomness setting 
      then convert it to ms so we can use fclamp to clamp it within the correct range */
  if (rnd_size_>0){
    float rnd_size_factor = fmap(RngFloat(),1.0f-rnd_size_, 1.0f+rnd_size_, Mapping::EXP);
    float size_ms = SamplesToMs(grain_size_) * rnd_size_factor;
    size_ms = fclamp(size_ms, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
    grain_size_ = MsToSamples(size_ms);
  }
  
  /* here we map a random num between -1/+1 and multiply by the audio sample 
      length and user random setting to add spray to grain spawn position
      if it overruns, wrap it back round to start/end of audio  */
  if (rnd_spawn_pos_>0){
    size_t rnd_offset = static_cast<size_t>(fmap(RngFloat(), -1.0f, 1.0f, Mapping::LINEAR));
    spawn_pos_ += audio_len_ * static_cast<size_t>(rnd_spawn_pos_ * rnd_offset);
    intclamp(spawn_pos_, 0, audio_len_-1);
  }

  if (rnd_pan_>0){
    float rnd_pan_offset = fmap(RngFloat(), -rnd_pan_, rnd_pan_, Mapping::LINEAR);
    pan_ = fclamp(pan_+rnd_pan_offset, 0.0f, 1.0f);
  }

  if (rnd_pitch_>0){
    float rnd_pitch_factor = fmap(RngFloat(), 1.0f-rnd_pitch_, 1.0+rnd_pitch_, Mapping::EXP);
    pitch_ratio_ = fclamp((pitch_ratio_*rnd_pitch_factor), MIN_PITCH, MAX_PITCH);
  }

  if (rnd_count_>0){
    rnd_count_/=2;
    float rnd_count_factor = fmap(RngFloat(),1.0f-rnd_count_,1.0f+rnd_count_, Mapping::LINEAR);
    active_count_ *= static_cast<size_t>(rnd_count_factor);
    intclamp(active_count_, MIN_GRAINS, MAX_GRAINS);
  }
}

/// @brief Triggers new grains and applies randomness to grain parameters
void GranularSynth::TriggerGrain(){
  size_t count = 0;
  for(Grain& grain:grains_){
    if (grain.is_active_) { count++; }
    else if (count<active_count_){
      // ApplyRandomness();
      // grain.SetEnvelopeType(env_type_);
      UpdateGrainParams();
      grain.Trigger(spawn_pos_,grain_size_,pitch_ratio_,pan_,direction_);
      count++;
      break;
    }
  }
}

/// @brief Processes and sums audio of active grains, then mixes to output buffers
/// @param out_left Pointer to left channel audio output buffer
/// @param out_right Pointer to right channel audio output buffer
/// @param size Number of samples to process in this call 
Sample GranularSynth::ProcessGrains(){
  sample_.left=0.0f, sample_.right=0.0f;
  TriggerGrain();
  // int cnt=0;
  for (Grain& grain:grains_){
    if (grain.is_active_){
      // cnt++;
      sample_ = grain.Process(sample_,1.0f);
    }
  }
  count++;
  if (count%60000==0){
    count=0;
    // DebugPrint(pod_, "lbuf %f, rbuf %f, envv %f",grains_[0].lbuf, grains_[0].rbuf, grains_[0].envv);
    DebugPrint(pod_, "sampL %f, sampR%f", sample_.left, sample_.right);
    // DebugPrint(pod_, "activecount %u pan %f",active_count_,pan_); 
  }



  
  // sample_.left = SoftClip(sample_.left);
  // sample_.right = SoftClip(sample_.right);

  return sample_;
}



