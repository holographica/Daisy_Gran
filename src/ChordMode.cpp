#include "ChordMode.h"

void ChordMode::Init(){
  AddScalePitchSet(ScaleType::Major, "Major", {0,2,4,5,7,9,11,12});
  AddScalePitchSet(ScaleType::NaturalMinor, "Natural Minor", {0,2,3,5,7,8,10});
}

/* convert pitch class (0[C]:11[B]) to freq ratio rel to base note */
float ChordMode::PitchClassToRatio(int pitch_class){
  return pow(2.0f, static_cast<float>(pitch_class)/12.0f);
}

/* transpose freq ratio of note to current key */
float ChordMode::MapKeyToRatio(float ratio){
  float key_ratio = static_cast<float>(curr_key_) / 12.0f;
  return ratio * std::pow(2.0f, key_ratio);
}

/* assumes scales added by this method are all of 12-tone equal temperament */
void ChordMode::AddScalePitchSet(ScaleType type, const string &name, const vector<int> &pitch_class_set){
  Scale scale;
  scale.equal_temperament_ = true;
  scale.name_ = name;
  for (uint16_t i=0; i<pitch_class_set.size();i++){
    scale.ratios_.emplace_back(PitchClassToRatio(pitch_class_set[i]));
  }
  int scale_type = static_cast<int>(type);
  scales_[scale_type] = scale;
}

/* assumes scales added by ratios are not 12-tone equal temperament */
void ChordMode::AddScaleRatios(ScaleType type, const string &name, const vector<float> &ratios){
  Scale scale;
  scale.equal_temperament_ = false;
  scale.name_ = name;
  scale.ratios_ = ratios;
  int scale_type = static_cast<int>(type);
  scales_[scale_type] = scale;
}

/* degree == note pos in scale. degree 1 = root note, 2 = second note etc 
  ensures we can use diff types of chord eg chord containing more notes than the scale */
float ChordMode::GetNotePitch(int degree){
  degree %= curr_scale_.ratios_.size();
  float ratio = curr_scale_.ratios_[degree];
  if (curr_scale_.equal_temperament_) return MapKeyToRatio(ratio);
  else return ratio;
}

/* atm this just creates a chord of thirds */
vector<float> ChordMode::GenerateChord(){
  vector<float> chord;
  vector<int> degrees;
  for (int i=0; i<curr_chord_size_; i++){
    int third = (i*2) % curr_scale_.ratios_.size();
    degrees.emplace_back(third);
  }

  /* convert degrees of chord to pitch ratios */
  for (size_t i=0; i<degrees.size();i++){
    chord.emplace_back(GetNotePitch(degrees[i]));
  }

  // randomise note order of chord here??
  
  return chord; 
}

void ChordMode::SetKey(int key){
  curr_key_ = key % 12;
}

void ChordMode::SetScale(int scale){
  // curr_scale_ = static_cast<Scale>(scale % NUM_SCALES);
}

void ChordMode::SetChordSize(int size){
  if (size < 2) { size = 2; }
  if (size > 12) { size = 12; }
  curr_chord_size_ = size;
}

void ChordMode::SetKeyRnd(float key_randomness){
  key_rnd_ = key_randomness;
}

void ChordMode::SetScaleRnd(float scale_randomness){
  scale_rnd_ = scale_randomness;
}

void ChordMode::SetChordSizeRnd(float sz_randomness){
  chord_size_rnd_ = sz_randomness;
}

// std::vector<float> chord;

  // calculate ratios / degrees of chord

  // apply randomness??

  // convert to ratio

  // convert to key? 


/*


*/


