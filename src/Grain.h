#pragma once

#include <stdint.h>
#include <stddef.h>

class Grain {
  public:
    Grain() {};
    ~Grain () {};

    void Init(float sample_rate);
    void Trigger(float size, float start_position, float pitch);

    float Process(float* sample_buffer, uint32_t buffer_size);

    void SetSize(float size);
    void SetStartPosition(float start_position);
    void SetPitch(float pitch);
    // other fns like update phase increment, set other params
    bool IsActive();
    void DeactivateGrain();


  private:

  // do i need a param (struct) for stereo samples??
  // ie left/right floats 
  // not sure if should be public or private

    // audio sample rate
    float sample_rate_;
    // 
    float sample_buffer_;
    size_t buffer_size_;

    // ie length, duration of grain
    float size_;
    /* position within audio sample/buffer 
       at which grain starts playback */
    float start_pos_;
    // ref to current absolute position within audio buffer
    float curr_pos_;
    // pitch is exposed to user - how many semitones shifted from original sample
    float pitch_;
    bool is_active_;

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

    - (amplitude) envelope
      - ie ADSR 
      - helps avoid clicks/pops by smoothing playback start/stop 
      - shapes the sound of the grain
      - represented as a single float value == an amplitude multiplier
        - this is calculated using ADSR values, curve shapes, etc

    */



};
