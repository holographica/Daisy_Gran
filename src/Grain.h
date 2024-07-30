#pragma once

// #include <stdio.h>
#include <stdint.h>
// #include <stddef.h>
#include "daisysp.h"
// #include "daisy_pod.h"
#include "audio_constants.h"
#include "GrainPhasor.h"

// using namespace daisy;
using namespace daisysp;

struct Sample {
  float left;
  float right;
};

class Grain {
  public:
    enum class EnvelopeType {
      /* Simple linear fade out ie: |\  */
      LinearDecay,
      /* Smooth symmetric increase/decrease
        ie linear fade in and out: /\  */
      Triangular,
      /* smooth curve on/off, crosses 0 at both sides */
      Hann,
      // could add option for ADSR where user sets params? 
    };

    // Grain(): pan_(0.5f), size_(0.0f), spawn_pos_(0.0f), is_active_(false), envelope_(EnvelopeType::Linear) {}
    Grain(): is_active_(false), envelope_type_(EnvelopeType::LinearDecay), envelope_phase_(0.0f) {}
    ~Grain() {};

    // init with default values: 0.5s grain size, centre pan
    void Init(){
      Init(500.0f, 0.5f);
    }

    void Init(float size_ms, float pan){
      phasor_.Init(0,1,GrainPhasor::Mode::OneShot);
      SetSize(MsToSamples(size_ms));
      SetPan(pan);
    }

    void Trigger(float size_ms, float spawn_pos, float pitch_ratio, GrainPhasor::Mode mode){
      if (size_ms <= 0 || pitch_ratio <=0 || spawn_pos <0){
        // invalid! 
        DeactivateGrain();
        return;
      }
      SetSpawnPos(spawn_pos);
      SetSize(MsToSamples(size_ms));
      phasor_.Init(size_ms, pitch_ratio, mode);
      envelope_phase_ =0.0f;
      ActivateGrain();
    }

    Sample Process(const int16_t* left_buf, const int16_t* right_buf){
      if (!is_active_) return { 0.0f,0.0f };
      float phase = phasor_.Process();
      /* find current position within overall audio sample */
      float curr_pos = spawn_pos_ + (phase*size_);
      /* mod by buffer size to keep within bounds */
      size_t buf_idx = static_cast<size_t>(curr_pos) % CHNL_BUF_SIZE_SAMPS;

      float left = s162f(left_buf[buf_idx]);
      float right = s162f(right_buf[buf_idx]);
      float env = ApplyEnvelope(phase);

      /* approximate constant power panning - cheaper than using sin/cos
      https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/panlaws.pdf
      works because gain_l^2 + gain_r^2 = 1   */
      float gain_left = Fast_Sqrt(1.0f-pan_);
      float gain_right = Fast_Sqrt(pan_);
      Sample out = {(left*env*gain_left),
                    (right*env*gain_right)};

      /* keep envelope phase synced with grain phase */
      envelope_phase_ = phase;
      if (phase>=1.0f) { DeactivateGrain(); }
      return out;
    }

    bool IsActive() const { return is_active_; }
    void SetPan(float new_pan) { pan_ = fclamp(new_pan,0.0f,1.0f); }
    void SetSize(float new_size) { size_ = new_size; }
    void SetSpawnPos(float new_pos) { spawn_pos_ = new_pos; };
    void SetPitchRatio(float ratio) { phasor_.SetPitchRatio(ratio, size_); }
    void SetEnvelopeType(EnvelopeType type) { envelope_type_ = type; }
    void SetPhasorMode(GrainPhasor::Mode mode) { phasor_.SetMode(mode); }
    void ResetPhasor() { phasor_.Reset(); }
    void ActivateGrain() { is_active_ = true; }
    void DeactivateGrain() { is_active_ = false; }

  private:
    /* simple phasor that handles phase management + incrementation */
    GrainPhasor phasor_;
    /* linear pan: 0 = fully left, 1 = fully right */
    float pan_;
    /* length of grain in samples */
    float size_;
    /* position in samples within audio buffer 
      at which grain starts playback */
    float spawn_pos_;
    bool is_active_;
    EnvelopeType envelope_type_;
    float envelope_phase_;


    float ApplyEnvelope(float phase){
      switch(envelope_type_){
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



    // loop time? ie grain (0.5s long) playing for 5s - could be ping pong
    
    /*
    - (amplitude) envelope
      - ie ADSR 
      - helps avoid clicks/pops by smoothing playback start/stop 
      - shapes the sound of the grain
      - represented as a single float value == an amplitude multiplier
        - this is calculated using ADSR values, curve shapes, etc
      
      - options
      - built in ADSR class
      - Hann aka Hanning - raised cosine
      - Hamming - slightly diff to Hann
      - Gaussian
    */


};