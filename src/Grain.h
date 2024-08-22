#pragma once

#include "daisy_pod.h"
#include "daisysp.h"
#include "sample.h"
#include "GrainPhasor.h"

using namespace daisy;
using namespace daisysp;

class Grain {
  public:
    Grain():
      is_active_(false){}

    void Init();
    void Trigger(size_t pos, size_t grain_size, float pitch_ratio=1.0f, float pan=0.5f, float direction=0.0f);
    Sample Process(Sample sample);
  
    void SetSpawnPos(size_t spawn_pos);
    void SetGrainSize(size_t grain_size);
    void SetPitchRatio(float pitch_ratio);

    // void SetPhasorPitchRatio(float pitch_ratio);
    void SetPhasorDirection(float direction);

    static size_t audio_len_;
    static int16_t *left_buf_;
    static int16_t *right_buf_;
    bool is_active_;

    GrainPhasor phasor_;
    float lbuf = 0;
    float rbuf=0;
    float envv=0;
    size_t count = 0;
  private:
    /* Grain audio parameters */
    float pan_;
    size_t spawn_pos_;
    size_t grain_size_;
    float pitch_ratio_;
    /* Object that manages the phase of the grain */

    static const float start_decay_;
    static const float decay_rate_;
    
    float ApplyEnvelope(float phase);
    

};