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

    void Init(const int16_t *left, const int16_t *right, size_t audio_len){
      left_buf_ = left;
      right_buf_ = right;
      audio_len_ = audio_len;
      for (Grain& grain: grains_){
        grain.Init(left,right,audio_len,&pod_);
      }
      SeedRng(static_cast<uint32_t>(rand()));
    }

    void GranularSynth::SeedRng(uint32_t seed){
      rng_state_ = seed;
    }

    float RngFloat(){
      /* xorshift32 from https://en.wikipedia.org/wiki/Xorshift 
      for simple fast random floats */
      rng_state_ ^= rng_state_ << 13;
      rng_state_ ^= rng_state_ >> 17;
      rng_state_ ^= rng_state_ << 5;
      /* ensures output is between 0 and 1 */
      float out = static_cast<float>(rng_state_) / static_cast<float>(UINT32_MAX);
      return out;
    }


    /* these setters are for internal use */

    void SetGrainSize(float size_ms){
      size_ms = fclamp(size_ms, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
      grain_size_ = MsToSamples(size_ms);
    }

    /* don't need a separate user setter for this
      since pan range is already 0-1 */
    void SetPan(float pan){
      pan_ = pan;
    }

    void SetSpawnPosSamples(size_t pos){
      spawn_pos_ = std::min(pos, audio_len_-1);
    }

    void SetActiveGrains(size_t count){
      if (count<0) { count=0; }
      if (count>20) { count=20; }
      active_count_ = count;
    }

    void SetPitchRatio(float ratio){
      pitch_ratio_ = ratio;
    }


    /* these setters take a normalised value (ie float from 0-1) 
      from the user input knobs and convert this to the correct units */

    void SetUserGrainSize(float knob_val){
      float size_ms = fmap(knob_val, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS);
      SetGrainSize(size_ms);
    }

    void SetUserSpawnPos(float knob_val){
      // float npos = fclamp(knob_val, 0.0f, 1.0f);
      /* convert to samples */
      spawn_pos_ = static_cast<size_t>(knob_val * static_cast<float>(audio_len_-1));
    }

    void SetUserActiveGrains(float knob_val){
      float count = round(fmap(knob_val, 1.0f, 5.0f));
      SetActiveGrains(static_cast<size_t>(count));
    }

    void SetUserPitchRatio(float ratio){
      float pitch = fmap(ratio, 0.5, 2, daisysp::Mapping::LOG);
      SetPitchRatio(pitch);
    }

    void SetPhasorMode(GrainPhasor::Mode mode){
      phasor_mode_ = mode;
    }




    void TriggerGrain(){
      size_t count = 0;
      for(Grain& grain:grains_){
        if (grain.IsActive()) { count++; }
        if (!grain.IsActive() && count<active_count_){
          grain.Trigger(spawn_pos_,grain_size_,pitch_ratio_,pan_);
          break;
        }
      }
    }

    void ProcessGrains(float *out_left, float *out_right, size_t size){
      for (size_t i=0; i<size;i++){
        TriggerGrain();
        float sum_left = 0.0f;
        float sum_right = 0.0f;
        size_t active = 0;
        for (Grain& grain:grains_){
          if (grain.IsActive()){
            grain.Process(&sum_left,&sum_right);
            active++;
          }
        }
        if (active>0){
          out_left[i]=(sum_left/active);
          out_right[i]=(sum_right/active);
        } 
        else {
          out_left[i]=0.0f;
          out_right[i]=0.0f;
        }
      }
    }






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
