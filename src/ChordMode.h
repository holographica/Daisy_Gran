#pragma once

#include "daisysp.h"
#include <array>

using namespace daisysp;

// https://www.sacred-geometry.es/?q=en/content/proportion-musical-scales
/* https://ianring.com/musictheory/scales/finder/ */
          /* insanely cool site!! */
enum class ScaleType{
  Major,
  NaturalMinor,
  HarmonicMinor,
  MelodicMinor
};

enum class ChordType{
  Major,
  Major7th,
  Minor,
  Minor7th
};

enum class ChordPlaybackMode{
  Chord,
  Arpeggio,
  Scale
};

class ChordMode{
  public:
    ChordMode(){}

    void SetKey(size_t key);
    void CycleScale();
    void CycleChord();
    void CyclePlaybackMode();
    float SemitoneToRatio(size_t semitones);    

    void GenerateChord();
    void GenerateScale(size_t direction);
    void GenerateArp(size_t direction);

    std::vector<float> GetRatios(size_t direction);

    ChordPlaybackMode GetMode();
    void GetModeName();
    void GetScaleName();
    void GetChordName();
    size_t GetStep();

  private:
    static const int MAX_CHORD_NOTES = 8; 
    static const int MAX_SCALE_NOTES = 8;

    struct ChordState{
      ScaleType curr_scale_ = ScaleType::Major;
      ChordType curr_chord_ = ChordType::Major;
      ChordPlaybackMode playback_mode_ =  ChordPlaybackMode::Chord;
      size_t key_ = 0;
      /* current step in arpeggio / scale */
      size_t curr_step_ = 0; 
    };

    ChordState chord_state_;
    std::vector<float> ratios_;

    char s[10];

    const std::array<std::vector<size_t>, 4> chord_intervals_ = 
    {{
      {0,4,7}, /* Major triad */
      {0,3,7}, /* minor triad */
      {0,4,7,11}, /* Major 7th */
      {0,3,7,10} /* Minor 7th */
    }};

    const std::array<std::vector<size_t>,4> scale_intervals_ = 
    {{
      {0,2,4,5,7,9,11}, /* Major scale */
      {0,2,3,5,7,8,10}, /* Natural Minor scale */
      {0,2,3,5,7,8,11}, /* Harmonic Minor scale */
      {0,2,3,5,7,9,11} /* Melodic Minor scale */
    }};
};