#pragma once

#include "daisy_pod.h"
#include "daisysp.h"
#include "GrainPhasor.h"

using namespace daisy;
using namespace daisysp;

class Grain {
  public:
    /// @brief Enum class to store references to differen types of amplitude envelope
    enum class EnvelopeType {
      /* basic linear fade out from start ie |\ */
      LinearDecay,
      /* No fade in or out */
      Rectangular,
      /* Simple fade out starting at phase=0.8f ie: |‾\  */
      Decay,
      /* Smooth symmetric increase/decrease
        ie linear fade in and out: /\  */
      Triangular,
      /* smooth curve on/off, crosses 0 at both sides */
      Hann,
      // could add option for ADSR where user sets params? 
    };

    Grain():
      left_buf_(nullptr), right_buf_(nullptr), is_active_(false),
      audio_len_(0), envelope_type_(EnvelopeType::Decay){}

    /// @brief Initialise the Grain instance and its Phasor object 
    /// @param left Const pointer to the left audio data buffer
    /// @param right Const pointer to the right audio data buffer
    /// @param len Length in samples of the audio file loaded in the buffers
    void Init(const int16_t *left, const int16_t *right, size_t len){
      left_buf_ = left;
      right_buf_ = right;
      audio_len_ = len;
      SetPhasorMode(GrainPhasor::Mode::OneShot);
      phasor_.Init(0.0f, 1.0f, phasor_mode_);
    }

    /// @brief Causes a grain to start playing and assigns its parameters
    /// @param pos Spawn position of the grain, within the audio buffer 
    /// @param grain_size Length of the grain in samples
    /// @param pitch_ratio Pitch of the grain - 1 plays the grain at its regular pitch
    /// @param pan Position of the grain's audio output in the stereo field
    void Trigger(size_t pos, size_t grain_size, float pitch_ratio=1.0f, float pan=0.5f) {
      if (pos >= audio_len_){
        pos = pos - audio_len_;
      }
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
    void Process(float *sum_left, float *sum_right) {
      if (!is_active_) return;
      float phase = phasor_.Process();
      if (phasor_.GrainFinished()){
        DeactivateGrain();
        return;
      }
      size_t curr_idx = spawn_pos_ + static_cast<size_t>(phase*grain_size_*pitch_ratio_);
      
      // NOTE: CHANGED THIS TO WRAP AROUND SO CHANGE BACK IF NEEDED
      if (curr_idx>=audio_len_-1){
        curr_idx %= audio_len_;
        // DeactivateGrain();
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

      // if (phase>= 1.0f){
      //   DeactivateGrain();
      // }
    }

    void SetSpawnPos(size_t spawn_pos){ spawn_pos_ = spawn_pos; }
    void SetGrainSize(size_t grain_size) { grain_size_ = grain_size; }
    void SetPitchRatio(float pitch_ratio) { pitch_ratio_ = pitch_ratio; }
    void SetEnvelopeType(EnvelopeType type) { envelope_type_ = type; }

    void SetPhasorPitchRatio(float pitch_ratio) { phasor_.SetPitchRatio(pitch_ratio, audio_len_); }
    void SetPhasorMode(GrainPhasor::Mode mode) { phasor_.SetMode(mode); }

    bool IsActive() const { return is_active_; }
    void ActivateGrain() { is_active_ = true; }
    void DeactivateGrain() { is_active_ = false; }

  private:
    const int16_t *left_buf_;
    const int16_t *right_buf_;

    size_t audio_len_;

    /* Object that manages the phase of the grain */
    GrainPhasor phasor_;
    GrainPhasor::Mode phasor_mode_;

    /* Grain audio parameters */
    float pan_;
    size_t spawn_pos_;
    size_t grain_size_;
    float pitch_ratio_;
    EnvelopeType envelope_type_;
    bool is_active_;


    const float start_decay_ = 0.8f;
    const float decay_rate_ = 5.0f;

    /// @brief Applies selected amplitude envelope to grain based on its phase
    /// @param phase Current playback position of the grain within its lifetime (from 0 - 1)
    /// @return Amplitude of the grain after envelope has been applied
    float ApplyEnvelope(float phase){
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

};
