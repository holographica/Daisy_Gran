#include "ChordMode.h"



void ChordMode::CycleChord(){
  size_t mode_idx = static_cast<size_t>(chord_state_.curr_chord_);
  mode_idx++;
  if (mode_idx>3) mode_idx = 0;
  chord_state_.curr_chord_= static_cast<ChordType>(mode_idx);
}

void ChordMode::CycleScale(){
  size_t mode_idx = static_cast<size_t>(chord_state_.curr_scale_);
  mode_idx++;
  if (mode_idx>3) mode_idx = 0;
  chord_state_.curr_scale_= static_cast<ScaleType>(mode_idx);
}

/* sets the root note / key */
void ChordMode::SetKey(size_t key){
  chord_state_.key_ = key;
  // chord_state_.key_ = key%12;
}

void ChordMode::CyclePlaybackMode(){
  int mode_idx = static_cast<int>(chord_state_.playback_mode_);
  mode_idx++;
  if (mode_idx>2) mode_idx = 0;
  chord_state_.playback_mode_ = static_cast<ChordPlaybackMode>(mode_idx);
  chord_state_.curr_step_ = 0;
}

float ChordMode::SemitoneToRatio(size_t semitones){
  return pow(2.0f, ((chord_state_.key_ + semitones)/12.0f));
}

std::vector<float> ChordMode::GetRatios(size_t direction){
  switch(chord_state_.playback_mode_){
    case ChordPlaybackMode::Chord:
      GenerateChord();
      break;
    case ChordPlaybackMode::Scale:
      GenerateScale(direction);
      break;
    case ChordPlaybackMode::Arpeggio:
      GenerateArp(direction);
      break;
  }
  return ratios_;
}

/* returns ratios of all notes in chord */
void ChordMode::GenerateChord(){
  ratios_.clear();
  /* get semitone positions for current chord */
  std::vector<size_t> curr_chord = chord_intervals_[static_cast<size_t>(chord_state_.curr_chord_)];
  for (size_t i=0; i<curr_chord.size();i++){
    /* get the semitone position for this note in chord, eg 0 / 4 /7 etc
      and find transposition by adding this to root note of key */
    size_t semitones = curr_chord[i] + chord_state_.key_;
    /* we do 2 ^ (note/12) to get correct pitch ratio for note
      since 1 semitone increases freq by [freq * 2^(1/12)]  */
    float pitch_ratio = SemitoneToRatio(semitones);
    ratios_.push_back(pitch_ratio);
  }
}

/* returns ratio of next note in scale */
void ChordMode::GenerateScale(size_t direction){
  ratios_.clear();
  std::vector<size_t> curr_scale = scale_intervals_[static_cast<int>(chord_state_.curr_scale_)];
  /* increment by direction, make sure it stays within scale bounds */
  chord_state_.curr_step_ += direction;
  if (chord_state_.curr_step_ > curr_scale.size()){
    chord_state_.curr_step_ =0;
  }
  else if (chord_state_.curr_step_ < 0){
    chord_state_.curr_step_ = curr_scale.size()-1;
  }
  /* get semitone position of next note in scale */
  size_t next_ratio = SemitoneToRatio(chord_state_.curr_step_);
  ratios_.push_back(next_ratio);
}

/* returns ratio of next note in chord */
void ChordMode::GenerateArp(size_t direction){
  ratios_.clear();
  std::vector<size_t> curr_chord = scale_intervals_[static_cast<size_t>(chord_state_.curr_chord_)];
  /* ensure it stays within chord bounds  */
  chord_state_.curr_step_ += direction;
  if (chord_state_.curr_step_ > curr_chord.size()){
    chord_state_.curr_step_ =0;
  }
  else if (chord_state_.curr_step_ < 0){
    chord_state_.curr_step_ = curr_chord.size()-1;
  }
  /* get semitone position of next note in chord */
  size_t next_ratio = SemitoneToRatio(chord_state_.curr_step_);
  ratios_.push_back(next_ratio);
}

ChordPlaybackMode ChordMode::GetMode(){
  return chord_state_.playback_mode_;
}

std::string ChordMode::GetModeName(){
  std::string s;
  switch(chord_state_.playback_mode_){
    case ChordPlaybackMode::Arpeggio:
      s = "switched to Arpeggio mode";
      break;
    case ChordPlaybackMode::Chord:
      s = "switched to Chord mode";
      break;
    case ChordPlaybackMode::Scale:
      s = "switched to Scale mode";
      break;
  }
  return s;
}

std::string ChordMode::GetScaleName(){
  std::string s;
  switch(chord_state_.curr_scale_){
    case ScaleType::Major:
      s = "switched to Major scale";
    case ScaleType::NaturalMinor:
      s = "switched to Natural Minor scale";
    case ScaleType::HarmonicMinor:
      s = "switched to Harmonic Minor scale";
    case ScaleType::MelodicMinor:
      s = "switched to Melodic Minor scale";
  }
  return s;
}

std::string ChordMode::GetChordName(){
  std::string s;
  switch (chord_state_.curr_chord_){
    case ChordType::Major:
      s = "switched to Major chord";
    case ChordType::Major7th:
      s = "switched to Maj7th chord";
    case ChordType::Minor:
      s = "switched to Minor chord";
    case ChordType::Minor7th:
      s = "switched to Min7th chord";
  }
  return s;
}

size_t ChordMode::GetStep(){
  return chord_state_.curr_step_;
}