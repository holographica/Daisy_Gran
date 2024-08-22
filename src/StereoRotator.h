#include <cmath>

class StereoRotator{
  public:
    StereoRotator(){}

    void SetFreq(float freq){
      freq_ = freq;
    }

    Sample Process(Sample in){
      rotation_  += (2*M_PI*freq_)/SAMPLE_RATE_FLOAT;
      if (rotation_ >2*M_PI){
        rotation_  -= 2*M_PI;
      }
      Sample out;
      out.left = (in.left*cos(rotation_)) - (in.right*sin(rotation_));
      out.right =(in.left*sin(rotation_)) - (in.right*cos(rotation_));
      return out;
    }

    Sample ProcessMix(Sample in){
      Sample out;
      Sample processed = Process(in);
      out.left = wet_mix_*processed.left + ((1.0f-wet_mix_)*in.left);
      out.right = wet_mix_*processed.right + ((1.0f-wet_mix_)*in.right);
      return out;
    }

    void SetMix(float mix) { wet_mix_ = mix; }

  private:
    float rotation_ =0.0f; /* angle of rotation */
    float freq_=0.1f; /* frequency of rotation in Hz*/
    float wet_mix_ =0.0f;
};