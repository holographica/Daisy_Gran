#include "ChordMode.h"



void ChordMode::CycleChord(){
  int mode_idx = static_cast<int>(chord_state_.curr_chord_);
  mode_idx++;
  if (mode_idx>3) mode_idx = 0;
  chord_state_.curr_chord_= static_cast<ChordType>(mode_idx);
}

void ChordMode::CycleScale(){
  int mode_idx = static_cast<int>(chord_state_.curr_scale_);
  mode_idx++;
  if (mode_idx>3) mode_idx = 0;
  chord_state_.curr_scale_= static_cast<ScaleType>(mode_idx);
}

/* sets the root note / key */
void ChordMode::SetKey(int key){
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

float ChordMode::SemitoneToRatio(int semitones){
  return pow(2.0f, ((chord_state_.key_ + semitones)/12.0f));
}

std::vector<float> ChordMode::GetRatios(int direction){
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

ChordPlaybackMode ChordMode::GetMode(){
  return chord_state_.playback_mode_;
}

std::string ChordMode::GetModeName(){
  switch (chord_state_.playback_mode_){
    case ChordPlaybackMode::Arpeggio:
      return "Arpeggio";
    case ChordPlaybackMode::Chord:
      return "Chord";
    case ChordPlaybackMode::Scale:
      return "Scale";
  }
  return "";
}

std::string ChordMode::GetScaleName(){
  switch(chord_state_.curr_scale_){
    case ScaleType::Major:
      return "Major";
    case ScaleType::NaturalMinor:
      return "NaturalMinor";
    case ScaleType::HarmonicMinor:
      return "HarmonicMinor";
    case ScaleType::MelodicMinor:
      return "MelodicMinor";
  }
  return "";
}

std::string ChordMode::GetChordName(){
  switch (chord_state_.curr_chord_){
    case ChordType::Major:
      return "maj";
    case ChordType::Major7th:
      return "maj7th";
    case ChordType::Minor:
      return "min";
    case ChordType::Minor7th:
      return "min7th";
  }
  return "";
}

/* returns ratios of all notes in chord */
void ChordMode::GenerateChord(){
  ratios_.clear();
  /* get semitone positions for current chord */
  std::vector<int> curr_chord = chord_intervals_[static_cast<int>(chord_state_.curr_chord_)];
  for (size_t i=0; i<curr_chord.size();i++){
    /* get the semitone position for this note in chord, eg 0 / 4 /7 etc
      and find transposition by adding this to root note of key */
    int semitones = curr_chord[i] + chord_state_.key_;
    /* we do 2 ^ (note/12) to get correct pitch ratio for note
      since 1 semitone increases freq by [freq * 2^(1/12)]  */
    float pitch_ratio = SemitoneToRatio(semitones);
    ratios_.push_back(pitch_ratio);
  }
}

/* returns ratio of next note in scale */
void ChordMode::GenerateScale(int direction){
  ratios_.clear();
  std::vector<int> curr_scale = scale_intervals_[static_cast<int>(chord_state_.curr_scale_)];
  /* increment by direction, make sure it stays within scale bounds */
  chord_state_.curr_step_ = (chord_state_.curr_step_+direction+curr_scale.size()) % curr_scale.size();
  /* get semitone position of next note in scale */
  int next_ratio = SemitoneToRatio(chord_state_.curr_step_);
  ratios_.push_back(next_ratio);
}

/* returns ratio of next note in chord */
void ChordMode::GenerateArp(int direction){
  ratios_.clear();
  std::vector<int> curr_chord = scale_intervals_[static_cast<int>(chord_state_.curr_chord_)];
  /* ensure it stays within chord bounds  */
  chord_state_.curr_step_ = (chord_state_.curr_step_+direction+curr_chord.size()) % curr_chord.size();
  /* get semitone position of next note in chord */
  int next_ratio = SemitoneToRatio(chord_state_.curr_step_);
  ratios_.push_back(next_ratio);
}
