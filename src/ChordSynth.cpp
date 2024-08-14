#include "ChordSynth.h"

void ChordSynth::Init(){
  // init scales here? 
}


void ChordSynth::SetKey(int key){
  key_ = key % 12;
}

void ChordSynth::SetScale(int scale){
  curr_scale_ = static_cast<Scale>(scale % NUM_SCALES);
}

void ChordSynth::SetChordSize(int size){
  if (size < 2) { size = 2; }
  if (size > 12) { size = 12; }
  chord_size_ = size;
}

void ChordSynth::SetKeyRnd(float key_randomness){
  key_rnd_ = key_randomness;
}

void ChordSynth::SetScaleRnd(float scale_randomness){
  scale_rnd_ = scale_randomness;
}

void ChordSynth::SetChordSizeRnd(float sz_randomness){
  chord_size_rnd_ = sz_randomness;
}

std::vector<float> ChordSynth::GenerateChord(){
  std::vector<float> chord;

  // calculate ratios / degrees of chord

  // apply randomness??

  // convert to ratio

  // convert to key? 
  


  return chord;
}

float ChordSynth::MapKeyToRatio(float ratio){
  float key_ratio = static_cast<float>(key_) / 12.0f;
  return ratio * std::pow(2.0f, key_ratio);
}



