#pragma once

#include "Grain.h"
#include "daisy_pod.h"
#include <vector>

class GranularSynth{
  public:
    GranularSynth(DaisyPod& pod) 
      : pod_(pod), left_buf_(nullptr), right_buf_(nullptr), audio_len_(0),
        grains_(){}

    void Init(const int16_t *left, const int16_t *right, size_t audio_len);
    void InitParams();
    void UpdateGrainParams();
    void ApplyRandomness();
    void TriggerGrain();
    void ProcessGrains(float *out_left, float *out_right, size_t size);

    void SetEnvelopeType(Grain::EnvelopeType type);
    void SetPhasorMode(GrainPhasor::Mode mode);
    void SetPan(float pan);
    
    void SetGrainSize(float knob_val);
    void SetSpawnPos(float knob_val);
    void SetActiveGrains(float knob_val);
    void SetPitchRatio(float ratio);

    void SetSizeRnd(float rnd){ rnd_size_ = rnd; }
    void SetPositionRnd(float rnd){ rnd_spawn_pos_ = rnd; }
    void SetPitchRnd(float rnd){ rnd_pitch_ = rnd; }
    void SetCountRnd(float rnd){ rnd_count_ = rnd; }
    void SetPanRnd(float rnd){ rnd_pan_ = rnd; }
    void SetEnvRnd(float rnd){ rnd_envelope_ = rnd; }
    void SetPhasorRnd(float rnd){ rnd_phasor_ = rnd; }

  private:
    DaisyPod& pod_;
    /* pointers to SDRAM audio buffers */
    const int16_t *left_buf_;
    const int16_t *right_buf_;
    /* length of audio in samples */
    size_t audio_len_;
    Grain grains_[MAX_GRAINS];

    /* parameters affecting audio output */
    GrainPhasor::Mode phasor_mode_;
    Grain::EnvelopeType env_type_;
    size_t grain_size_;
    size_t spawn_pos_;
    size_t active_count_;
    float pitch_ratio_;
    float pan_;

    /* amount of randomness to apply to synth/grain parameters*/
    float rnd_size_ = 0.0f;
    float rnd_spawn_pos_ = 0.0f;
    float rnd_count_ = 0.0f;
    float rnd_pitch_ = 0.0f;
    float rnd_pan_ = 0.0f;
    float rnd_envelope_ = 0.0f;
    float rnd_phasor_ = 0.0f;




};
