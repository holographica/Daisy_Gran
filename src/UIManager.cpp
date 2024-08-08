#include "UIManager.h"

void UIManager::UpdateControls(){
  UpdateKnobs();
  UpdateEncoder();
  UpdateState();
  UpdateSynthMode();
}

void UIManager::UpdateKnobs(){
  int mode_idx = static_cast<int>(synth_mode_);
  float k1v = MapKnobDeadzone(GetKnob1Value());
  float k2v = MapKnobDeadzone(GetKnob2Value());

  /* here we pass the current knob value, pointer to the stored knob value for this mode,
    and a pointer to the bool which tracks whether the knob value has been passed through */
  k1v_[mode_idx] = UpdateKnobPassThru(k1v, &k1v_[mode_idx], &k1_pass_thru_[mode_idx]);
  k2v_[mode_idx] = UpdateKnobPassThru(k2v, &k2v_[mode_idx], &k2_pass_thru_[mode_idx]);
}

void UIManager::UpdateEncoder(){
  pod_.encoder.Debounce();
}

void UIManager::UpdateState(){
  pod_.button2.Debounce();
  if (Button2Pressed()){
    switch(current_state_){
      case AppState::Startup:
        current_state_ = AppState::SelectFile;
        break;
      case AppState::SelectFile:
        current_state_ = AppState::PlayWAV;
        break;
      case AppState::PlayWAV:
        current_state_ = AppState::Synthesis;
        break;
      case AppState::Synthesis:
        // here i need to change the substates
        // i'll use a different control to switch from synthesis mode back to file select
        UpdateSynthMode();
        break;

    }
  }
}

void UIManager::UpdateSynthMode(){
  switch(synth_mode_){
    case SynthMode::SizePosition:
      synth_mode_ = SynthMode::PitchGrains;
      break;
    case SynthMode::PitchGrains:
      synth_mode_ = SynthMode::SizePosition;
      break;
    // add more when i add more modes
  }
}

float UIManager::GetKnob1Value(){
  return pod_.knob1.Process();
}

float UIManager::GetKnob2Value(){
  return pod_.knob2.Process();
}

int32_t UIManager::GetEncoderIncrement(){
  return pod_.encoder.Increment();
}

bool UIManager::EncoderPressed(){
  return pod_.encoder.FallingEdge();
}

bool UIManager::Button1Pressed(){
  return pod_.button1.FallingEdge();
}

bool UIManager::Button2Pressed(){
  return pod_.button2.FallingEdge();
}

void UIManager::SetLed1(int r, int g, int b){
  pod_.led1.Set(r,g,b);
  pod_.UpdateLeds();
  System::Delay(100);
}

void UIManager::BlinkLed1(int r, int g, int b){
  SetLed1(r,g,b);
  SetLed1(0,0,0);
}

void UIManager::BlinkSetLed1(int r, int g, int b){
  BlinkLed1(r,g,b);
  BlinkLed1(r,g,b);
  SetLed1(r,g,b);
}


float UIManager::MapKnobDeadzone(float knob_val ){
  if (knob_val<=0.01f) return 0.0f;
  if (knob_val>=0.99f) return 1.0f;
  return knob_val;
}

/* here we check if the current value of the knob has moved through the last stored value 
  before the synth mode changed, and only update the parameter once the current knob value
  'passes through' the stored value - meaning if we set grain size to 0.1 then switch modes 
  and move the knob to 0.8, if we switch back, grain size won't be updated again until the 
  knob passes through this value.  */

float UIManager::UpdateKnobPassThru(float curr_knob_val, float *stored_knob_val, bool *pass_thru){
  if (!(*pass_thru)){
    if (curr_knob_val >= (*stored_knob_val) || curr_knob_val <= (*stored_knob_val)) {
      (*pass_thru) = true;
    }
} 
  if (*pass_thru){
    (*stored_knob_val) = curr_knob_val;
    return curr_knob_val;
  }
  return (*stored_knob_val);
}