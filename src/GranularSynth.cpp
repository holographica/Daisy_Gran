#include "GranularSynth.h"

using namespace daisy;

/* define all static variables before initialisation */

const int16_t* GranularSynth::left_buf_ = nullptr;
const int16_t* GranularSynth::right_buf_ = nullptr;
/* length of audio in samples */
DTCMRAM_BSS size_t GranularSynth::audio_len_;
DTCMRAM_BSS Grain GranularSynth::grains_[MAX_GRAINS];

/* parameters affecting audio output */
DTCMRAM_BSS GrainPhasor::Mode GranularSynth::phasor_mode_;
DTCMRAM_BSS Grain::EnvelopeType GranularSynth::env_type_;
DTCMRAM_BSS size_t GranularSynth::grain_size_;
DTCMRAM_BSS size_t GranularSynth::spawn_pos_;
DTCMRAM_BSS size_t GranularSynth::active_count_;
DTCMRAM_BSS float GranularSynth::pitch_ratio_;
DTCMRAM_BSS float GranularSynth::pan_;

/* amount of randomness to apply to synth/grain parameters*/
DTCMRAM_BSS float GranularSynth::rnd_size_;
DTCMRAM_BSS float GranularSynth::rnd_spawn_pos_;;
DTCMRAM_BSS float GranularSynth::rnd_count_;
DTCMRAM_BSS float GranularSynth::rnd_pitch_;
DTCMRAM_BSS float GranularSynth::rnd_pan_;
DTCMRAM_BSS float GranularSynth::rnd_envelope_;
DTCMRAM_BSS float GranularSynth::rnd_phasor_;


/// @brief Initialise granular synth object and assign audio buffers
/// @param left Left channel audio data buffer
/// @param right Right channel audio data buffer
/// @param audio_len Length of currently loaded audio file in samples
void GranularSynth::Init(DaisyPod& pod, const int16_t *left, const int16_t *right, size_t audio_len){
  pod_ = &pod;
  left_buf_ = left;
  right_buf_ = right;
  audio_len_ = audio_len;
  for (Grain& grain: grains_){
    grain.Init();
  }
  Grain::left_buf_ = left;
  Grain::right_buf_ = right;
  Grain::audio_len_ = audio_len;

  /* initialise grain params */
  phasor_mode_ = GrainPhasor::Mode::OneShot;
  env_type_ = Grain::EnvelopeType::Decay;
  grain_size_ = 4800;
  spawn_pos_ = 0;
  active_count_ =1;
  pitch_ratio_ = 1.0f;
  pan_ = 0.5f;

  /* initialise random params */
  rnd_size_=0.0f;
  rnd_spawn_pos_=0.0f;
  rnd_count_=0.0f;
  rnd_pitch_=0.0f;
  rnd_pan_=0.0f;
  rnd_envelope_=0.0f;
  rnd_phasor_=0.0f;
}

void GranularSynth::SetEnvelopeType(Grain::EnvelopeType type){ 
  env_type_ = type; 
  // DebugPrint(pod_, "set env to %u", static_cast<int>(type));
}

void GranularSynth::SetPhasorMode(GrainPhasor::Mode mode){ 
  phasor_mode_ = mode;
  // DebugPrint(pod_, "set phasor to %u", static_cast<int>(mode));
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
ITCMRAM_TEXT void GranularSynth::UpdateGrainParams(){
  for (Grain& grain:grains_){
    grain.SetGrainSize(grain_size_);
    grain.SetSpawnPos(spawn_pos_);
    grain.SetPitchRatio(pitch_ratio_);
  }
}

/// @brief We take a random number, map it into a certain range
///        determined by the user randomness setting for that parameter,
///        then clamp it so it stays in the correct range for the parameter 

ITCMRAM_TEXT void GranularSynth::ApplyRandomness(){
  /* here we map a randomly generated num between 1 +/- user randomness setting 
      then convert it to ms so we can use fclamp to clamp it within the correct range */
  if (rnd_size_>0){
    float rnd_size_factor = fmap(RngFloat(),1.0f-rnd_size_, 1.0f+rnd_size_, Mapping::EXP);
    float size_ms = SamplesToMs(grain_size_) * rnd_size_factor;
    size_ms = fclamp(size_ms, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
    grain_size_ = MsToSamples(size_ms);
  }
  
  /* here we map a random num between -1/+1 and multiply by the audio length
      and user random setting to add spray to grain spawn position
      if it overruns, wrap it back round to start/end of audio  */
  if (rnd_spawn_pos_>0){
    float rnd_offset = rnd_spawn_pos_ * (fmap(RngFloat(), -1.0f, 1.0f, Mapping::LINEAR));
    spawn_pos_ += audio_len_ * static_cast<size_t>(rnd_offset);
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

  // if (rnd_envelope_>0){
  //   if (RngFloat() < rnd_envelope_){
  //     int random_env_type =  static_cast<int>(RngFloat()) * NUM_ENV_TYPES;
  //     env_type_ = static_cast<Grain::EnvelopeType>(random_env_type);
  //   }
  // }

  // if (rnd_phasor_>0){
  //   if (RngFloat() < rnd_phasor_){
  //     int random_phasor_mode = static_cast<int>(RngFloat()) * NUM_PHASOR_MODES;
  //     phasor_mode_ = static_cast<GrainPhasor::Mode>(random_phasor_mode);
  //   }
  // }

  if (rnd_count_>0){
    float rnd_count_factor = fmap(RngFloat(),1.0f-rnd_count_,1.0f+rnd_count_, Mapping::EXP);
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
      ApplyRandomness();
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
ITCMRAM_TEXT void GranularSynth::ProcessGrains(float *out_left, float *out_right, size_t size){
  for (size_t i=0; i<size;i++){
    TriggerGrain();
    float sum_left = 0.0f, sum_right = 0.0f;
    for (Grain& grain:grains_){
      if (grain.is_active_){
        grain.Process(&sum_left,&sum_right);
      }
    }
    out_left[i]=sum_left;
    out_right[i]=sum_right;
  }
}