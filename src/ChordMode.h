#pragma once

#include "daisysp.h"
#include <map>

using namespace daisysp;
using namespace std;

// https://www.sacred-geometry.es/?q=en/content/proportion-musical-scales
/* https://ianring.com/musictheory/scales/finder/ */
          /* insanely cool site!! */
enum class ScaleType{
  Major,
  NaturalMinor,
  HarmonicMinor,
  MinorPentatonic,
  // Hirajoshi (which mode)
  NUM_SCALES_ //NOTE don't need to change, just add scales above this
};

class ChordMode{
  public:
    ChordMode(){}

    void Init();  

    void SetKey(int key);
    void SetScale(int scale);
    void SetChordSize(int size);
    void SetKeyRnd(float randomness);
    void SetScaleRnd(float randomness);
    void SetChordSizeRnd(float randomness);
    // randomise position of notes in chord ? ie in output grains


    vector<float> GenerateChord();

  private:
    float MapKeyToRatio(float ratio);
    float PitchClassToRatio(int pitch_class);
    void AddScalePitchSet(ScaleType type, const string& name, const vector<int> &pitch_class_set);
    void AddScaleRatios(ScaleType type, const string &name, const vector<float> &ratios);
    float GetNotePitch(int degree);

    // static const int NUM_SCALES = 3; //NOTE change obviously
    static const int NUM_SCALES = static_cast<int>(ScaleType::NUM_SCALES_);
    static const int MAX_CHORD_NOTES = 8; 
    static const int MAX_SCALE_NOTES = 8;

    struct Scale{
      bool equal_temperament_;
      vector<float> ratios_;
      string name_;
    };


    array<Scale, NUM_SCALES> scales_;


    int curr_key_;
    Scale curr_scale_;
    int curr_chord_size_;

    // float chord_[MAX_CHORD_NOTES];
    vector<float> chord;
    // float scales_[NUM_SCALES][MAX_SCALE_NOTES] = {};

    float key_rnd_;
    float scale_rnd_;
    float chord_size_rnd_;

    
    

};