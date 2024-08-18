#pragma once

#include "Grain.h"
#include "daisy_pod.h"
// #include "stddef.h"
#include "debug_print.h"
#include <bitset>

class GranularSynth{
  public:
    GranularSynth(): pod_(nullptr) {}
    // GranularSynth(DaisyPod& pod) 
    //   : pod_(pod){}

    void Init(DaisyPod& pod, const int16_t *left, const int16_t *right, std::size_t audio_len);
    void UpdateGrainParams();
    void ApplyRandomness();
    void TriggerGrain();
    void ProcessGrains(float *out_left, float *out_right, std::size_t size);

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
    DaisyPod* pod_;
    /* pointers to SDRAM audio buffers */
    static const int16_t *left_buf_;
    static const int16_t *right_buf_;
    /* length of audio in samples */
    static DTCMRAM_BSS std::size_t audio_len_;
    static Grain grains_[MAX_GRAINS];

    /* parameters affecting audio output */
    static DTCMRAM_BSS GrainPhasor::Mode phasor_mode_;
    static DTCMRAM_BSS Grain::EnvelopeType env_type_;
    static DTCMRAM_BSS std::size_t grain_size_;
    static DTCMRAM_BSS std::size_t spawn_pos_;
    static DTCMRAM_BSS std::size_t active_count_;
    static DTCMRAM_BSS float pitch_ratio_;
    static DTCMRAM_BSS float pan_;

    /* amount of randomness to apply to synth/grain parameters*/
    static DTCMRAM_BSS float rnd_size_;
    static DTCMRAM_BSS float rnd_spawn_pos_; 
    static DTCMRAM_BSS float rnd_count_;
    static DTCMRAM_BSS float rnd_pitch_;
    static DTCMRAM_BSS float rnd_pan_;
    static DTCMRAM_BSS float rnd_envelope_;
    static DTCMRAM_BSS float rnd_phasor_;
};
