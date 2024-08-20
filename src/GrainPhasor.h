#include "constants_utils.h"
class GrainPhasor {
  public:
    enum class Mode {
      OneShot,
      OneShotReverse,
      Cycle,
      PingPong,
    };

    GrainPhasor(): 
      phase_(0), increment_(0), mode_(Mode::OneShot), 
      direction_(1), grain_finished_(false) {};

    ~GrainPhasor() {};

    void Init(size_t grain_size, float pitch_ratio, Mode play_mode){
      increment_ = pitch_ratio/static_cast<float>(grain_size);
      mode_ = play_mode;
      phase_ = 0;
      direction_ = 1;
      if (mode_==Mode::OneShotReverse){
        phase_ = 1;
        direction_ = -1;
      }
      grain_finished_ = false;
    }

    void Reset(){
      phase_ =0;
      direction_ = 1;
    }

    void SetPitchRatio(float pitch_ratio, float grain_size_samples){
      float size_ms = SamplesToMs(grain_size_samples);
      increment_ = pitch_ratio / size_ms;
    }

    void SetMode(Mode new_mode){
      mode_ = new_mode;
    }

    void SetDirection(float direction){
      if (direction>0.5){
        direction_ = -1;
      }
      else direction = 1;
    }

    bool GrainFinished(){
      return grain_finished_;
    }

    float Process(){
      if (grain_finished_) { return 0.0f; }
      float out = phase_;
      out += increment_*direction_;

      switch(mode_){
        /* phase increases 0 -> 1 then stops */
        case Mode::OneShot:
        default:
          if(out>1.0f) {
            grain_finished_ = true;
            out=1.0f;
          }
          if (out<0.0f){
            grain_finished_ = true;
            out=0.0f;
          }
          break;
        /* phase goes 1 -> 0 then stops */
        case Mode::OneShotReverse:
          direction_ = -1;
          if(out<0.0f) {
            out=0.0f;
            grain_finished_ = true;
          } 
          break;
        /* loops from 0 -> 1 repeatedly */
        case Mode::Cycle:
          if(out>1.0f) out -=1.0f;
          if (out<0.0f) out +=1.0f;
          break;
        /* loops repeatedly forwards from 0 -> 1
          then backwards from 1 -> 0 ...*/
        case Mode::PingPong:
          if (out>1.0f){
            out = 2.0f-out;
            direction_ = -1;
          } else if (out<0.0f){
            out= -out;
            direction_ = 1;
          }
          break;
      }
      phase_ = out;
      // phase_ += increment_*direction_;
      return out;
    }

  private:
    float phase_;
    /* phase increment - controls grain pitch/speed */
    float increment_;
    Mode mode_;
    /* linear playback direction
    1 is forwards, -1 is backwards */
    int direction_;
    bool grain_finished_;
};