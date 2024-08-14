#pragma once

#include "daisysp.h"

using namespace daisysp;

/* https://ianring.com/musictheory/scales/finder/ */
          /* insanely cool site!! */
enum class Scale{
  Major,
  NaturalMinor,
  HarmonicMinor,

  // Hirajoshi (which mode)
  // MinorPentatonic,
};

class ChordSynth{
  public:
    ChordSynth(){}

    void Init();

    void SetKey(int key);
    void SetScale(int scale);
    void SetChordSize(int size);
    void SetKeyRnd(float randomness);
    void SetScaleRnd(float randomness);
    void SetChordSizeRnd(float randomness);
    // randomise position of notes in chord ? ie in output grains


    std::vector<float> GenerateChord();
    float MapKeyToRatio(float ratio);
    // convert scale degrees (int) to ratios (float) ?

  private:
    
    static const int NUM_SCALES = 3; //NOTE change obviously
    static const int MAX_CHORD_NOTES = 8; 
    static const int MAX_SCALE_NOTES = 8;

    int key_;
    Scale curr_scale_;
    int chord_size_;

    float chord_[MAX_CHORD_NOTES];
    float scales_[NUM_SCALES][MAX_SCALE_NOTES] = {};

    float key_rnd_;
    float scale_rnd_;
    float chord_size_rnd_;

    
    

};