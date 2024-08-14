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
      // float size_ms = SamplesToMs(size_samples);

      // float size_samples = (SAMPLE_RATE/1000) * size_ms;
      // increment_ = pitch_ratio / size_samples;
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
      float size_ms = SamplesToMs(grain_size_samples); // NOTE: CHECK - THIS MIGHT NOT WORK PROPERLY
      increment_ = pitch_ratio / size_ms;
    }

    void SetMode(Mode new_mode){
      mode_ = new_mode;
    }

    bool GrainFinished(){
      return grain_finished_;
    }
    // bool ModeComplete() { 
    //   return (phase_>=1.0f) && (mode_==Mode::OneShot);
    // }

// TODO: do i need all these getters? 
    // float GetPhase() const { return phase_; }
    // float GetIncrement() const { return increment_; }
    // Mode GetMode() const { return mode_; }
    // int GetDirection() const { return direction_; }

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

/*
    - phase
      - == current pos within grain waveform - from 0 to sample length
      - determines which part of sample is played 
      - relative position within grain 
    
    - phase increment
      - this is calculated based on pitch as well as sample rate + other factors
      - (basically how fast we move thru buffer when playing the grain)

      - amount phase is increased at each step of processing
        - ie every audio sample == data point - 1 sample is 1/48000 secs at 48kHz
      - ie controls speed + pitch of grain
      - large inc == faster speed so higher pitch
        - eg if phase inc==1.0: phase moves 1 sample position each processing step
        - if ==0.5, every sample/data point is read twice
          - this means overalll audio file is played at half speed -> pitch is 1 octave lower
        - if ==2, every other data point is skipped -> 2x speed -> 1octave higher

        - roughly (check this): phase increment = (source_sample_rate / output_sample_rate) * 2^(pitch_shift_in_semitones/12)
        - use source+output SRs if we allow playback at diff rate than original sample
        - ie if file is 48khz, output is 44.1
        - depends if i convert files to a set SR or not 
        - helps with flexibility in regards to input/output devices

*/