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

    void SetKey(int key);
    void CycleScale();
    void CycleChord();
    // void SetChordSize(int size);
    void CyclePlaybackMode();
    // void SetKeyRnd(float randomness);
    // void SetScaleRnd(float randomness);
    // void SetChordSizeRnd(float randomness);
    // randomise position of notes in chord ? ie in output grains
    float SemitoneToRatio(int semitones);

    void GenerateChord();
    void GenerateScale(int direction);
    void GenerateArp(int direction);

    std::vector<float> GetRatios(int direction);
    ChordPlaybackMode GetMode();
    std::string GetModeName();
    std::string GetScaleName();
    std::string GetChordName();

  private:
    static const int MAX_CHORD_NOTES = 8; 
    static const int MAX_SCALE_NOTES = 8;

    struct ChordState{
      ScaleType curr_scale_ = ScaleType::Major;
      ChordType curr_chord_ = ChordType::Major;
      ChordPlaybackMode playback_mode_ =  ChordPlaybackMode::Chord;
      int key_ = 0;
      /* current step in arpeggio / scale */
      int curr_step_ = 0; 
    };

    ChordState chord_state_;
    std::vector<float> ratios_;

    const std::array<std::vector<int>, 4> chord_intervals_ = 
    {{
      {0,4,7}, /* Major triad */
      {0,3,7}, /* minor triad */
      {0,4,7,11}, /* Major 7th */
      {0,3,7,10} /* Minor 7th */
    }};

    const std::array<std::vector<int>,4> scale_intervals_ = 
    {{
      {0,2,4,5,7,9,11}, /* Major scale */
      {0,2,3,5,7,8,10}, /* Natural Minor scale */
      {0,2,3,5,7,8,11}, /* Harmonic Minor scale */
      {0,2,3,5,7,9,11} /* Melodic Minor scale */
    }};
};