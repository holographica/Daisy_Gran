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
      is_active_(false), envelope_type_(EnvelopeType::Decay){}

    void Init();
    void Trigger(size_t pos, size_t grain_size, float pitch_ratio=1.0f, float pan=0.5f);
    void Process(float *sum_left, float *sum_right);
  
    void SetSpawnPos(size_t spawn_pos);
    void SetGrainSize(size_t grain_size);
    void SetPitchRatio(float pitch_ratio);
    void SetEnvelopeType(EnvelopeType type);

    void SetPhasorPitchRatio(float pitch_ratio);
    void SetPhasorMode(GrainPhasor::Mode mode);

    static size_t audio_len_;
    static const int16_t *left_buf_;
    static const int16_t *right_buf_;
    bool is_active_;

  private:
    /* Object that manages the phase of the grain */
    GrainPhasor phasor_;
    GrainPhasor::Mode phasor_mode_;

    /* Grain audio parameters */
    float pan_;
    size_t spawn_pos_;
    size_t grain_size_;
    float pitch_ratio_;
    EnvelopeType envelope_type_;

    static const float start_decay_;
    static const float decay_rate_;
    
    float ApplyEnvelope(float phase);
    

};
