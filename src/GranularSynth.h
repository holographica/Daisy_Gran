#pragma once

#include "Grain.h"
#include "daisy_pod.h"
#include <vector>

class GranularSynth{
  public:
    GranularSynth(DaisyPod& pod): 
      pod_(pod), left_buf_(nullptr), right_buf_(nullptr), audio_len_(0),
      grain_size_(4800), spawn_pos_(0), 
      active_count_(1), pitch_ratio_(1.0f), pan_(0.5f),
      grains_(), phasor_mode_(GrainPhasor::Mode::OneShot){}

    void Init(const int16_t *left, const int16_t *right, size_t audio_len);
    void SeedRng(uint32_t seed);
    float RngFloat();

    void SetPan(float pan);
    void SetPhasorMode(GrainPhasor::Mode mode);
    /* internal setters */

    void SetGrainSize(float size_ms);
    void SetSpawnPosSamples(size_t pos);
    void SetActiveGrains(size_t count);
    void SetPitchRatio(float ratio);
    
    /* user setters that take normalised input */

    void SetUserGrainSize(float knob_val);
    void SetUserSpawnPos(float knob_val);
    void SetUserActiveGrains(float knob_val);
    void SetUserPitchRatio(float ratio);

    void UpdateGrainParams();
    void TriggerGrain();
    void ProcessGrains(float *out_left, float *out_right, size_t size);






// void SetSpray(float range){
    //   spray_range_ = audio_len_ * static_cast<size_t>(range);
    // }

    // void UpdateSpray(){
    //   spray_range_ = std::min(grain_size_*2, audio_len_);
    // }



  private:
    DaisyPod& pod_;
    const int16_t *left_buf_;
    const int16_t *right_buf_;
    size_t audio_len_;
    size_t grain_size_;
    size_t spawn_pos_;
    size_t active_count_;
    float pitch_ratio_;
    float pan_;
    float density_;
    Grain grains_[MAX_GRAINS];
    GrainPhasor::Mode phasor_mode_;
    uint32_t rng_state_;
    // float spray_range_;
};
