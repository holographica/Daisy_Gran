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
    grain.Init(left,right);
  }
  Grain::audio_len_ = audio_len;
  Grain::left_buf_ = left;
  Grain::right_buf_ = right;
  InitParams();
}

void GranularSynth::Reset(size_t len){
  audio_len_ = len;
  for (Grain& grain: grains_){
    grain.Init(left_buf_,right_buf_);
  }
  InitParams();
}

/// @brief  Set intial grain parameter values
void GranularSynth::InitParams(){
  phasor_mode_ = GrainPhasor::Mode::OneShot;
  grain_size_ = 4800;
  spawn_pos_ = 0;
  active_count_ = 1;
  pitch_ratio_ = 1.0f;
  pan_ = 0.5f;
}

void GranularSynth::SetEnvelopeType(Grain::EnvelopeType type){
  // if (type==Grain::EnvelopeType::LinearDecay){
  //   DebugPrint(pod_, "set env to linear decya");
  // }
  // if (type==Grain::EnvelopeType::Decay){
  //   DebugPrint(pod_, "set env to decay");
  // }
  // if (type==Grain::EnvelopeType::Rectangular){
  //   DebugPrint(pod_, "set env to rect");
  // }
  // if (type==Grain::EnvelopeType::Hann){
  //   DebugPrint(pod_, "set env to hann");
  // }
  env_type_ = type; 
}

void GranularSynth::SetPhasorMode(GrainPhasor::Mode mode){
  // if (mode==GrainPhasor::Mode::OneShot){
  //   DebugPrint(pod_, "set phasor to oneshot");
  // }
  // if (mode==GrainPhasor::Mode::OneShotReverse){
  //   DebugPrint(pod_, "set phasor to reverse");
  // }
  // if (mode==GrainPhasor::Mode::Cycle){
  //   DebugPrint(pod_, "set phasor to cycle");
  // }
  // if (mode==GrainPhasor::Mode::PingPong){
  //   DebugPrint(pod_, "set phasor to pingpong");
  // }
  phasor_mode_ = mode;
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

/// @brief Update grain audio parameters
void GranularSynth::UpdateGrainParams(){
  for (Grain& grain:grains_){
    grain.SetGrainSize(grain_size_);
    grain.SetSpawnPos(spawn_pos_);
    grain.SetPitchRatio(pitch_ratio_);
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
      grain.SetPhasorMode(phasor_mode_);
      grain.Trigger(spawn_pos_,grain_size_,pitch_ratio_,pan_);
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

  /* apply normalisation to grain, scaled by number of active grains */
  // float normalise_per_grain = 1.0f/daisysp::fmax(1.0f, fastroot(active_count_,1));
  int count = 0;
  for (Grain& grain:grains_){
    if (grain.is_active_){
      count++;
      // sample_ = grain.Process(sample_, normalise_per_grain);
      sample_ = grain.Process(sample_,1);
      // sample_ = grain.Process(sample_, normalise_per_grain);
    }
  }

  /* attempt to normalise overall gain */
  // float normalise_output = 1.0f/daisysp::fmax(1.0f, fastlog2f(active_count_+1));
  // sample_.left *= normalise_output;
  // sample_.right *= normalise_output;

  // NOTE: try above and below see which is better
  
  // sample_.left = SoftClip(sample_.left*normalise_output);
  // sample_.right = SoftClip(sample_.right*normalise_output);
  
  // sample_.left = SoftClip(sample_.left);
  // sample_.right = SoftClip(sample_.right);

  return sample_;
}



