#pragma once

#include "Grain.h"
#include "daisy_pod.h"
#include "debug_print.h"
#include <vector>

class GranularSynth{
  public:
    // GranularSynth(DaisyPod& pod) 
    GranularSynth(DaisyPod& pod, SmoothRandomGenerator* rng) 
      : pod_(pod), left_buf_(nullptr), right_buf_(nullptr), audio_len_(0),
        grains_(), rng_(rng){}
        // grains_(){}

    void Init(int16_t *left, int16_t *right, size_t audio_len);
    void Reset(size_t len);
    void InitParams();
    void UpdateGrainParams();
    void TriggerGrain();
    Sample ProcessGrains();

    void SetPan(float pan);    
    void SetGrainSize(float knob_val);
    void SetSpawnPos(float knob_val);
    void SetTargetActiveGrains(float knob_val);
    void UpdateActiveGrains();
    size_t GetActiveGrains();
    void SetActiveGrains(float count);
    void SetPitchRatio(float ratio);
    void SetDirection(float direction);

    void SetRndAmount(float amount);
    void SetRndBias(float bias);
    float GetRnd();

    /* used to set chorus pan */
    float GetPan(){ return pan_; }
    size_t GetSize(){ return grain_size_; }
    float GetPitch(){ return pitch_ratio_; }
    size_t GetPos() { return spawn_pos_; }
    size_t GetCount(){ return curr_active_count_; }

  private:
    DaisyPod& pod_;
    /* pointers to SDRAM audio buffers */
    int16_t *left_buf_;
    int16_t *right_buf_;
    /* length of audio in samples */
    size_t audio_len_;
    Grain grains_[MAX_GRAINS];
    Sample sample_;

    size_t count=0;
    SmoothRandomGenerator* rng_;

    /* parameters affecting audio output */
    size_t grain_size_;
    size_t spawn_pos_;
    size_t curr_active_count_;
    size_t target_active_count_;

    float pitch_ratio_;
    float pan_;
    float direction_;

    /* amount of randomness to apply to synth/grain parameters*/
    float rnd_amount_;
    float rnd_bias_;
};
