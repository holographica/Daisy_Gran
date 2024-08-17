#include "Grain.h"


size_t Grain::audio_len_ = 0;
const int16_t* Grain::left_buf_ = 0;
const int16_t* Grain::right_buf_ = 0;


/// @brief Initialise the Grain instance and its Phasor object 
/// @param left Const pointer to the left audio data buffer
/// @param right Const pointer to the right audio data buffer
/// @param len Length in samples of the audio file loaded in the buffers
void Grain::Init(){
// void Grain::Init(const int16_t *left, const int16_t *right){
  // left_buf_ = left;
  // right_buf_ = right;
  SetPhasorMode(GrainPhasor::Mode::OneShot);
  phasor_.Init(0.0f, 1.0f, phasor_mode_);
}

/// @brief Causes a grain to start playing and assigns its parameters
/// @param pos Spawn position of the grain, within the audio buffer 
/// @param grain_size Length of the grain in samples
/// @param pitch_ratio Pitch of the grain - 1 plays the grain at its regular pitch
/// @param pan Position of the grain's audio output in the stereo field
void Grain::Trigger(size_t pos, size_t grain_size, float pitch_ratio, float pan) {
  if (pos >= audio_len_) pos -= audio_len_;
  spawn_pos_ = pos;
  grain_size_ = grain_size;
  is_active_ = true;
  phasor_.Init(grain_size, pitch_ratio, phasor_mode_);
  pitch_ratio_ = pitch_ratio;
  pan_ = pan;
}

/// @brief Processes grain phase, applies envelope and panning and mixes into the output buffers
/// @param sum_left Pointer to variable storing total left channel audio output of all grains
/// @param sum_right Pointer to variable storing total right channel audio output of all grains
void Grain::Process(float *sum_left, float *sum_right) {
  if (!is_active_) return;
  float phase = phasor_.Process();
  if (phasor_.GrainFinished()){
    is_active_=false;
    return;
  }
  size_t curr_idx = spawn_pos_ + static_cast<size_t>(phase*grain_size_*pitch_ratio_);
  
  // NOTE: CHANGED THIS TO WRAP AROUND SO CHANGE BACK IF NEEDED
  if (curr_idx>=audio_len_-1){
    curr_idx %= audio_len_;
    // return;
  }
  else if (curr_idx<0){
    curr_idx += audio_len_;
    // return;
  }

  float left = s162f(left_buf_[curr_idx]);
  float right = s162f(right_buf_[curr_idx]);
  float env = ApplyEnvelope(phase);

  /* approximate constant power panning - cheaper than using sin/cos
  this works since gain_l^2 + gain_r^2 = 1
  source: cs.cmu.edu/~music/icm-online/readings/panlaws/panlaws.pdf */
  float gain_left = std::sqrt(1.0f-pan_);
  float gain_right = std::sqrt(pan_);
  *sum_left += (left*env*gain_left);
  *sum_right += (right*env*gain_right);
}

/// @brief Applies selected amplitude envelope to grain based on its phase
/// @param phase Current playback position of the grain within its lifetime (from 0 - 1)
/// @return Amplitude of the grain after envelope has been applied
float Grain::ApplyEnvelope(float phase){
  switch(envelope_type_){
    case EnvelopeType::LinearDecay:
      return 1.0f - phase;
    case EnvelopeType::Rectangular:
      return phase;
    case EnvelopeType::Decay:
    default:
      if (phase<=start_decay_) { return 1.0f; }
      else { return 1.0f - ((phase - start_decay_) * decay_rate_); }
    case EnvelopeType::Triangular:
      return 1.0f - std::abs(2.0f*phase - 1.0f);
    /* formula from https://uk.mathworks.com/help/signal/ref/hann.html */
    case EnvelopeType::Hann:
      return 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
  }
}


void Grain::SetSpawnPos(size_t spawn_pos){ spawn_pos_ = spawn_pos; }
void Grain::SetGrainSize(size_t grain_size) { grain_size_ = grain_size; }
void Grain::SetPitchRatio(float pitch_ratio) { pitch_ratio_ = pitch_ratio; }
void Grain::SetEnvelopeType(EnvelopeType type) { envelope_type_ = type; }

void Grain::SetPhasorPitchRatio(float pitch_ratio) { phasor_.SetPitchRatio(pitch_ratio, audio_len_); }
void Grain::SetPhasorMode(GrainPhasor::Mode mode) { phasor_.SetMode(mode); }

// bool Grain::IsActive() { return is_active_; }