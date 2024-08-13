#pragma once

#include "daisy_pod.h"
#include "daisysp.h"
#include "GrainPhasor.h"

using namespace daisy;
using namespace daisysp;

class Grain {
  public:
    enum class EnvelopeType {
      /* basic linear fade out from start */
      LinearDecay,
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
      pod_(nullptr), left_buf_(nullptr), right_buf_(nullptr), is_active_(false),
      // audio_len_(0), phase_(0), phase_increment_(0), envelope_type_(EnvelopeType::Decay){}
      audio_len_(0), envelope_type_(EnvelopeType::Decay){}

    void Init(const int16_t *left, const int16_t *right, size_t len, DaisyPod *pod){
      left_buf_ = left;
      right_buf_ = right;
      audio_len_ = len;
      pod_ = pod;
      SetPhasorMode(GrainPhasor::Mode::OneShot);
      phasor_.Init(0.0f, 1.0f, phasor_mode_);
    }

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
    DaisyPod* pod_;
    const int16_t *left_buf_;
    const int16_t *right_buf_;
    bool is_active_;
    size_t audio_len_;
    GrainPhasor phasor_;
    GrainPhasor::Mode phasor_mode_;
    // float phase_;
    // float phase_increment_;
    float pan_;
    size_t spawn_pos_;
    size_t grain_size_;
    float pitch_ratio_;
    EnvelopeType envelope_type_;

    const float start_decay_ = 0.8f;
    const float decay_rate_ = 5.0f;

    float ApplyEnvelope(float phase){
      switch(envelope_type_){
        case EnvelopeType::Decay:
          if (phase<=start_decay_) { return 1.0f; }
          else { return 1.0f - ((phase - start_decay_) * decay_rate_); }
        case EnvelopeType::Triangular:
          return 1.0f - std::abs(2.0f*phase - 1.0f);
        /* formula from https://uk.mathworks.com/help/signal/ref/hann.html */
        case EnvelopeType::Hann:
          return 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
        case EnvelopeType::LinearDecay:
        default:
          return 1.0f - phase;
      }
    }

};
