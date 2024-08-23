#include "constants_utils.h"
class GrainPhasor {
  public:

    GrainPhasor(): 
      phase_(0.0f), increment_(0.0f), grain_finished_(false) {};

    ~GrainPhasor() {};

    void Init(size_t grain_size, float pitch_ratio){
      increment_ = pitch_ratio/static_cast<float>(grain_size);
      phase_ = 0.0f;
      grain_finished_ = false;
    }

    void Reset(){
      phase_ = 0.0f;
    }

    bool GrainFinished(){
      return grain_finished_;
    }

    float Process(){
      if (grain_finished_) { return 0.0f; }
      phase_ = phase_ + increment_;
      if (phase_>1.0f){
        grain_finished_ = true;
        phase_ =0.0f;
      }
      return phase_;
    }

  private:
    float phase_;
    /* phase increment - controls grain pitch/speed */
    float increment_;
    bool grain_finished_;
};