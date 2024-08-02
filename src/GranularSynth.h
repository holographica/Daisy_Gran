#include <stdio.h>
#include <algorithm>
#include "daisy_pod.h"
#include "Grain.h"
#include "constants.h"

using namespace daisy;

class GranularSynth {
public:
  GranularSynth(DaisyPod *pod): left_channel_(nullptr), right_channel_(nullptr), pod_(pod) {};
  ~GranularSynth() {};

  void Init(int16_t *left, int16_t *right, size_t file_len_samples);
  int GetAvailableGrainIdx() const;
  void TriggerGrain();
  void ProcessGrains(float *out_left, float *out_right, size_t size);
  // get random value function for randomness params? 

  int16_t* GetLeftBuffer() const { return left_channel_; }
  int16_t* GetRightBuffer() const { return right_channel_; }
  float GetDensity() const { return density_; }
  float GetSize() const { return size_; }
  float GetPan() const { return pan_; }
  float GetSpawnPos() const { return spawn_pos_; }
  float GetPitch() const { return pitch_ratio_; }
  
  float MapKnobDeadzone(float knob_val);
  void SeedRng(uint32_t seed);
  float RngFloat();

  // setters
  void UpdateMaxDensity();
  void SetSize(float size_ms);
  void SetDensity(float density);
  void SetPan(float pan);
  void SetSpawnPos(float pos);
  void SetPitch(float ratio);
  void SetPhasorMode(GrainPhasor::Mode mode);






private:
  DaisyPod* pod_;
  int16_t* left_channel_;
  int16_t* right_channel_;
  size_t file_len_samples_;
  float len_samples_float;

  // std::vector<Grain> grains_;
  Grain grains_[MAX_GRAINS];
  GrainPhasor::Mode curr_phasor_mode_;


// NOTE: could use a grain config struct eg
// struct GrainConfig {
//   float density;
//   float size;
//   float pan;
//   float spawn_pos;
//   float spray;
//   float pitch_ratio;
// };
// then use like "cfg_.density" etc



  /* how often in time grains are sampled from the audio buffer
    range: 40 grains/second to 2000 grains/second 
    (based on max no of grains and grain size)*/
  float density_;
  /* length of grains in ms: range 10ms to 1s */
  // NOTE: should i keep this in samples (like grain) or ms (user facing)??
  float size_; 
  /* range 0-1: 0 is fully left, 0.5 is centre, 1 is fully right */
  float pan_;
  /* centre position within the audio buffer for spawn/playback of new grains (range 0-1)
    it's the centre position since due to spray, spawn pos is not fixed */
  float spawn_pos_; 
  /* determines how far from spawn_pos a grain can start (range 0-1)
    note this is a random offset ie generates poss start values around centre */
  float spray_;

  float pitch_ratio_;

  /* number of samples since last grain was triggered */
  float since_last_grain_;
  /* tracks maximum possible density based on current grain size */
  float max_density_;
  float rand_;
  uint32_t rng_state_;



  // NOTE: not using this since i have grain phasor modes
  // /* Changes grain spawn pos with each new grain: 
  //   range -1 (backwards) to 1 (forwards) */ 
  // float scan_;

};










/*

params: 
- vector of grains 
- float array for audio buffer (> vector? consider performance vs ease)
- buffer size
- synth params eg density, spray, grain size etc
- last grain time? 


functions:
- set audio buffer
- process grains
- set [param] eg (global - all grains) density, grain size, pitch, spray etc
- update grains? (private - shouldn't be accessible outside this class)
  - handles triggering new grains, resetting them after they finish, managing grain lifetimes   

- function to check if we should trigger new grain? incl in update grains? 
- ie triggers grains regularly based on density (add randomness)


*/