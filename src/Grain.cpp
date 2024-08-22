#include "Grain.h"

size_t Grain::audio_len_;
int16_t* Grain::left_buf_;
int16_t* Grain::right_buf_;

const float Grain::start_decay_ = 0.8f;
const float Grain::decay_rate_ = 5.0f;

/// @brief Initialise the Grain instance and its Phasor object 
/// @param len Length in samples of the audio file loaded in the buffers
void Grain::Init(){
  phasor_.Init(0.0f, 1.0f, 0.0f);
}

/// @brief Causes a grain to start playing and assigns its parameters
/// @param pos Spawn position of the grain, within the audio buffer 
/// @param grain_size Length of the grain in samples
/// @param pitch_ratio Pitch of the grain - 1 plays the grain at its regular pitch
/// @param pan Position of the grain's audio output in the stereo field
void Grain::Trigger(size_t pos, size_t grain_size, float pitch_ratio, float pan, float direction) {
  if (pos >= audio_len_) pos -= audio_len_;
  // spawn_pos_ = pos;
  SetSpawnPos(pos);
  SetGrainSize(grain_size);
  SetPitchRatio(pitch_ratio);
  pan_=pan;
  // grain_size_ = grain_size;
  is_active_ = true;
  phasor_.Init(grain_size_, pitch_ratio_, 1.0f);
  // pitch_ratio_ = pitch_ratio;
  // pan_ = pan;
}

/// @brief Processes grain phase, applies envelope and panning and mixes into the output buffers
/// @param sum_left Pointer to variable storing total left channel audio output of all grains
/// @param sum_right Pointer to variable storing total right channel audio output of all grains
Sample Grain::Process(Sample sample) {
  count++;
  if (!is_active_) {
      return {-0.42069f,-0.42069f};
  }
  float phase = phasor_.Process();
  if (phasor_.GrainFinished()){
    is_active_=false;
    return {-0.42069f,-0.42069f};
  }
  size_t curr_idx = spawn_pos_ + static_cast<size_t>(phase*grain_size_*pitch_ratio_);
  
  if (curr_idx>=audio_len_-1){
    curr_idx %= audio_len_;
  }
  else if (curr_idx<0){
    curr_idx += audio_len_;
  }

  float left = s162f(left_buf_[curr_idx]);
  lbuf = left;
  float right = s162f(right_buf_[curr_idx]);
  rbuf = right;
  float env = ApplyEnvelope(phase);
  envv = env;

  /* approximate constant power panning - cheaper than using sin/cos
  this works since gain_l^2 + gain_r^2 = 1
  source: cs.cmu.edu/~music/icm-online/readings/panlaws/panlaws.pdf */
  float gain_left = std::sqrt(1.0f-pan_);
  float gain_right = std::sqrt(pan_);
  sample.left += (left*env*gain_left);
  sample.right += (right*env*gain_right);
  return sample;
}

/// @brief Applies amplitude envelope (Hann window) to grain based on its phase
/// @param phase Current playback position of the grain within its lifetime (from 0 - 1)
/// @return Amplitude of the grain after envelope has been applied
float Grain::ApplyEnvelope(float phase){
  /* Hann formula from https://uk.mathworks.com/help/signal/ref/hann.html */
  // return 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
  return 1.0f;
}

void Grain::SetSpawnPos(size_t spawn_pos){ spawn_pos_ = spawn_pos; }
void Grain::SetGrainSize(size_t grain_size) { grain_size_ = grain_size; }
void Grain::SetPitchRatio(float pitch_ratio) { pitch_ratio_ = pitch_ratio; }
// void Grain::SetPhasorPitchRatio(float pitch_ratio) { phasor_.SetPitchRatio(pitch_ratio, audio_len_); }
void Grain::SetPhasorDirection(float direction) { phasor_.SetDirection(direction); }
