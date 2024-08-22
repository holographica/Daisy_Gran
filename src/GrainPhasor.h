#include "constants_utils.h"
class GrainPhasor {
  public:

    GrainPhasor(): 
      phase_(0), increment_(0), direction_(1.0f), grain_finished_(false) {};

    ~GrainPhasor() {};

    void Init(size_t grain_size, float pitch_ratio, int direction){
      increment_ = pitch_ratio/static_cast<float>(grain_size);
      phase_ = 0;
      SetDirection(direction);
      grain_finished_ = false;
    }

    void Reset(){
      phase_ = 0;
      direction_ = 1;
    }

    /* set playback direction to forward if knob is over halfway point */
    void SetDirection(float direction){
      direction_ = direction>0.5 ? -1.0f: 1.0f;
    }

    bool GrainFinished(){
      return grain_finished_;
    }

    float Process(){
      if (grain_finished_) { return 0.0f; }
      phase_ = phase_ + (increment_ * direction_);
      if (phase_ >1.0f && direction_ == 1){
        grain_finished_=true;
        phase_ =0.0f;
      }
      if (phase_ <0.0f && direction_ == -1){
        grain_finished_=true;
        phase_  = 1.0f;
      }
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