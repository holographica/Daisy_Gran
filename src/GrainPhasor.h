#include "constants_utils.h"
class GrainPhasor {
  public:

    GrainPhasor(): 
      phase_(0), increment_(0), direction_(1.0f), grain_finished_(false) {};

    ~GrainPhasor() {};

    void Init(size_t grain_size, float pitch_ratio, int direction){
      // SetPitchRatio(pitch_ratio, grain_size);
      increment_ = pitch_ratio/static_cast<float>(grain_size);
      phase_ = 0;
      SetDirection(direction);
      grain_finished_ = false;
    }

    void Reset(){
      phase_ = 0;
      direction_ = 1;
    }

    // void SetPitchRatio(float pitch_ratio, float grain_size_samples){
    //   float size_ms = SamplesToMs(grain_size_samples);
    //   increment_ = pitch_ratio / size_ms;
    // }

    /* set playback direction to forward if knob is over halfway point */
    void SetDirection(float direction){
      direction_ = direction>0.5 ? -1.0f: 1.0f;
    }

    bool GrainFinished(){
      return grain_finished_;
    }

    float Process(){
      if (grain_finished_) { return 0.0f; }
      // float out = phase_;
      // out += increment_;
      phase_ = phase_ + increment_;

      if (phase_ > 1.0f){
        grain_finished_=true;
        phase_=0.0f;
      }
      // if (out>1.0f && direction_==1){
      //   grain_finished_=true;
      //   out=0.0f;
      // }
      // if (out<0.0f && direction_==-1){
      //   grain_finished_=true;
      //   out = 1.0f;
      // }

      // phase_ = out;
      return phase_;
    }

  private:
    float phase_;
    /* phase increment - controls grain pitch/speed */
    float increment_;
    /* linear playback direction
    1 is forwards, -1 is backwards */
    float direction_;
    bool grain_finished_;
};