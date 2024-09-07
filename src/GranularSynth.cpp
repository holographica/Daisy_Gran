#include "GranularSynth.h"

using namespace daisy;

/// @brief Initialise granular synth object and assign audio buffers
/// @param left Left channel audio data buffer
/// @param right Right channel audio data buffer
/// @param audio_len Length of currently loaded audio file in samples
void GranularSynth::Init(int16_t *left, int16_t *right, size_t audio_len){
  left_buf_ = left;
  right_buf_ = right;
  audio_len_ = audio_len;
  for (Grain& grain: grains_){
    grain.Init();
  }
  Grain::audio_len_ = audio_len;
  Grain::left_buf_ = left;
  Grain::right_buf_ = right;
  InitParams();
}

void GranularSynth::Reset(size_t len){
  audio_len_ = len;
  for (Grain& grain: grains_){
    grain.Init();
  }
  InitParams();
}

/// @brief  Set intial grain parameter values
void GranularSynth::InitParams(){
  grain_size_ = 4800;
  spawn_pos_ = 0;
  curr_active_count_ = 1;
  pitch_ratio_ = 1.0f;
}

/* these setters take a normalised value (ie float from 0-1) 
  from the user input knobs and convert this to the correct units */

void GranularSynth::SetGrainSize(float knob_val){
  float rnd = (RngFloat() * 0.1f) + knob_val;
  knob_val = fclamp(rnd, 0.0f, 1.0f);
  // knob_val = fclamp(knob_val, 0.0f, 1.0f);
  float size_ms = fmap(knob_val, MIN_GRAIN_SIZE_MS, MAX_GRAIN_SIZE_MS, daisysp::Mapping::LOG);
  grain_size_ = MsToSamples(size_ms);
}

void GranularSynth::SetSpawnPos(float knob_val){
  float rnd = (RngFloat() * 0.1f) + knob_val;
  knob_val = fclamp(rnd, 0.0f, 1.0f);
  // knob_val = fclamp(knob_val, 0.0f, 1.0f);
  /* convert to samples */
  spawn_pos_ = static_cast<size_t>(knob_val * static_cast<float>(audio_len_-1));
}

void GranularSynth::SetPitchRatio(float ratio){
  float rnd = (RngFloat() * 0.1f) + ratio;
  ratio = fclamp(rnd, 0.0f, 1.0f);
  // ratio = fclamp(ratio, 0.0f, 1.0f);
  pitch_ratio_ = fmap(ratio, 0.5, 2, daisysp::Mapping::LINEAR);
}

void GranularSynth::SetTargetActiveGrains(float knob_val){
  knob_val = round(fmap(knob_val, 1.0f, 10.0f));
  target_active_count_ = (static_cast<size_t>(knob_val));
}

/* this increases the number of active grains more smoothly - had dropout issues without */
void GranularSynth::UpdateActiveGrains(){
  if (curr_active_count_< target_active_count_){
    smooth_count ++;
    if (smooth_count==48000){
      smooth_count = 0;
      curr_active_count_++;
    }
  }
}

size_t GranularSynth::GetActiveGrains(){
  return curr_active_count_;
}

/// @brief Triggers new grains 
void GranularSynth::TriggerGrain(){
  size_t count = 0;
  for(Grain& grain:grains_){
    if (grain.is_active_) { count++; }
    else if (count<curr_active_count_){
      size_t pos = spawn_pos_ + static_cast<size_t>((static_cast<float>(spawn_pos_)*RngFloat())*0.2f);
      pos = intclamp(pos, 0.0f, audio_len_);
      size_t sz = grain_size_ + static_cast<size_t>((static_cast<float>(grain_size_)*RngFloat())*0.2f);
      sz = intclamp(sz, MIN_GRAIN_SIZE_SAMPLES, MAX_GRAIN_SIZE_SAMPLES);
      float pitch = fclamp(pitch_ratio_ + (pitch_ratio_*RngFloat()*0.2f), 0.5f, 2.0f);
      grain.Trigger(pos,sz,pitch);
      count++;
      break;
    }
  }
}

/// @brief Processes and sums audio of active grains, then mixes to output buffers
/// @param out_left Pointer to left channel audio output buffer
/// @param out_right Pointer to right channel audio output buffer
/// @param size Number of samples to process in this call 
Sample GranularSynth::ProcessGrains(){
  UpdateActiveGrains();
  sample_.left=0.0f, sample_.right=0.0f;
  TriggerGrain();
  for (Grain& grain:grains_){
    if (grain.is_active_){
      sample_ = grain.Process(sample_);
    }
  }
  return sample_;
}

void GranularSynth::EnqueueChord(std::vector<float> ratios){
  chord_queue_.push(ratios);
}

bool GranularSynth::ChordActive(){
  return chord_active_;
}

bool GranularSynth::ChordQueueEmpty(){
  return chord_queue_.empty();
}

void GranularSynth::TriggerChord(){
  if (chord_queue_.empty()) return;
  chord_ratios_ = chord_queue_.front();
  chord_queue_.pop();
  chord_active_ = true;
  chord_grain_lengths_.clear();
  max_chord_length_ = 0;
  chord_sample_count_ = 0;
  for (size_t i=0; i<chord_ratios_.size(); i++){
    size_t grain_len = static_cast<size_t>(grain_size_/chord_ratios_[i]);
    chord_grain_lengths_.push_back(grain_len);
    max_chord_length_ = std::max(max_chord_length_, grain_len);
    grains_[i].Trigger(spawn_pos_, grain_size_, chord_ratios_[i]);
  }
}

Sample GranularSynth::ProcessChord(){
  sample_ = {0.0f, 0.0f};
  if (chord_active_){
    bool grains_finished = true;
    for (size_t i =0; i<chord_ratios_.size();i++){
      if (chord_sample_count_ < chord_grain_lengths_[i]){
        sample_ = grains_[i].Process(sample_);
        grains_finished = false;
      }
    }
    chord_sample_count_++;
    if (grains_finished || (chord_sample_count_ >= max_chord_length_)){
      chord_active_ =false;
    }
  }
  return sample_;
}


