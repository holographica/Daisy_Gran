#include <cmath>

class StereoRotator{
  public:
    StereoRotator(){}

    void SetFreq(float freq){
      /* map to 0.01Hz to 1.0Hz - don't want it too fast */
      freq_ = fmap(freq, 0.01f, 0.5f, daisysp::Mapping::EXP);
    }

    Sample Process(Sample in){
      /* increment the rotation in the stereo field */
      rotation_  += (2*M_PI*freq_)/SAMPLE_RATE_FLOAT;
      /* normalise angle to keep within bounds of +- pi for accurate-ish(?) results */
      rotation_ = NormaliseRotationAngle(rotation_);

      Sample out;
      /* from https://en.wikipedia.org/wiki/Rotation_matrix:
        treat left as x coord, right as y coord to rotate around centre
      */
      float cos_rotation = FastCos(rotation_);
      float sin_rotation = FastSin(rotation_);
      out.left = (in.left*cos_rotation) - (in.right*sin_rotation);
      out.right =(in.left*sin_rotation) - (in.right*cos_rotation);
      return out;
    }

    /* process the input then mix by the wet/dry amount controlled by knob */
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