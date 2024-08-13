#pragma once

#include "Grain.h"
#include "daisy_pod.h"
#include <vector>

class GranularSynth{
  public:
    GranularSynth(DaisyPod& pod) 
      : pod_(pod), left_buf_(nullptr), right_buf_(nullptr), audio_len_(0),
        grains_(), phasor_mode_(GrainPhasor::Mode::OneShot),
        grain_size_(4800), spawn_pos_(0), 
        active_count_(1), pitch_ratio_(1.0f), pan_(0.5f){}

    void Init(const int16_t *left, const int16_t *right, size_t audio_len);
    void SeedRng(uint32_t seed);
    float RngFloat();

    
    /* internal setters */

    void SetGrainSize(float size_ms);
    void SetSpawnPosSamples(size_t pos);
    void SetActiveGrains(size_t count);
    void SetPitchRatio(float ratio);
    void SetEnvelopeType(Grain::EnvelopeType type){ env_type_ = type; }
    void SetPhasorMode(GrainPhasor::Mode mode){ phasor_mode_ = mode;}
    /* don't need a user setter for this
    since pan range is already 0-1 */
    void SetPan(float pan){ pan_ = pan; }
    
    /* user setters that take normalised input */

    void SetUserGrainSize(float knob_val);
    void SetUserSpawnPos(float knob_val);
    void SetUserActiveGrains(float knob_val);
    void SetUserPitchRatio(float ratio);
    

    void UpdateGrainParams();
    void ApplyRandomness();
    void TriggerGrain();
    void ProcessGrains(float *out_left, float *out_right, size_t size);

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
    size_t audio_len_;
    Grain grains_[MAX_GRAINS];
    GrainPhasor::Mode phasor_mode_;
    Grain::EnvelopeType env_type_;
    uint32_t rng_state_;

    /* synth parameters affecting audio output */
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
