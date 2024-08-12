#include "UIManager.h"

void UIManager::UpdateControls(){
  // pod_.ProcessAllControls();
  UpdateKnobs();
  // UpdateEncoder();
  UpdateState();
  // UpdateSynthMode();
}

void UIManager::UpdateKnobs(){
  if (current_state_ == AppState::Synthesis){
    int mode_idx = static_cast<int>(synth_mode_);
    float k1v = MapKnobDeadzone(GetKnob1Value());
    float k2v = MapKnobDeadzone(GetKnob2Value());

    /* here we pass the current knob value, pointer to the stored knob value for this mode,
      and a pointer to the bool which tracks whether the knob value has been passed through */
    k1v_[mode_idx] = UpdateKnobPassThru(k1v, &k1v_[mode_idx], &k1_pass_thru_[mode_idx]);
    k2v_[mode_idx] = UpdateKnobPassThru(k2v, &k2v_[mode_idx], &k2_pass_thru_[mode_idx]);
  }
}

// void UIManager::UpdateEncoder(){
//   pod_.encoder.Debounce();
// }

// use encoder to select states - led is flashing in state selection mode
// or could slowly go from low - high - low brightness / pulsing
//   startup led1 =white
//   selectfile led1 = blue
//   playwav led1 = cyan
//   synthesis led1 = green
//   error led1 = red

// once synth loads it will be set to solid colour showing the synth mode
// use button1 to select synth modes
//   green for first mode? so it's easy to remember flash green = synth, solid green = first mode
//   then maybe yellow / blue / purple or pink? 
// led could flash when a mode is changed? 
// use button2 to flip from controlling synth params to randomness of those params
//   in param control mode, led2 is green
//   in randomness mode, led2 is red




void UIManager::UpdateState(){
  pod_.ProcessDigitalControls();
  // pod_.button2.Debounce();
  // TODO: change this to encoder pressed
  if (Button1Pressed()){
    switch(current_state_){
      case AppState::Startup:
        // NOTE: state should auto change to select file once startup is finished
        // can set this in GranularApp once built
        current_state_ = AppState::SelectFile; 
        break;
      case AppState::SelectFile:
        current_state_ = AppState::PlayWAV;
        // NOTE: need to start audiocallback once file is successfully selected/loaded
        // NOTE: should i select next state (ie playwav) by clicking encoder? or pressing button1? 
        break;
      case AppState::PlayWAV:
        current_state_ = AppState::Synthesis;
        synth_mode_ = SynthMode::Size_Position;
        break;
      case AppState::Synthesis:
        UpdateSynthMode();
        break;
      case AppState::Error:
        // NOTE: do something here 
        // ie flash led1 red 
        // could flash led2 red if sd error, yellow if synth error etc etc
        break;
    }
  }
  if (Button2Pressed()){
    switch(current_state_){
      case AppState::Synthesis:
        ToggleRandomnessControls();
        break;
      default:
        break;
    }
  }
  /* exit synthesis mode, return to file selection, stop audio. */
  if (EncoderLongPress()){
    switch (current_state_){
      case AppState::Synthesis:
        pod_.StopAudio();
        current_state_ = AppState::SelectFile;
        break;
    }
  }
}

// TODO: change this to button 1 being pressed
// TODO: change randomness to button 2 pressed
void UIManager::UpdateSynthMode(){
  switch(synth_mode_){
    case SynthMode::Size_Position:
      synth_mode_ = SynthMode::Pitch_ActiveGrains;
      break;
    case SynthMode::Pitch_ActiveGrains:
      synth_mode_ = SynthMode::Pan_PanRnd;
      break;
    case SynthMode::Pan_PanRnd:
      synth_mode_ = SynthMode::PhasorMode_EnvType;
      break;
    case SynthMode::PhasorMode_EnvType:
      synth_mode_ = SynthMode::Size_Position;
      break;
    // add more when i add more modes
  }
}

void UIManager::ToggleRandomnessControls(){
  switch(synth_mode_){
    case SynthMode::Size_Position:
      synth_mode_ = SynthMode::Size_Position_Rnd;
      break;
    case SynthMode::Size_Position_Rnd:
      synth_mode_ = SynthMode::Size_Position;
      break;
    case SynthMode::Pitch_ActiveGrains:
      synth_mode_ = SynthMode::Pitch_ActiveGrains_Rnd;
      break;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      synth_mode_ = SynthMode::Pitch_ActiveGrains;
      break;
    case SynthMode::PhasorMode_EnvType:
      synth_mode_ = SynthMode::PhasorMode_EnvType_Rnd;
      break;      
    case SynthMode::PhasorMode_EnvType_Rnd:
      synth_mode_ = SynthMode::PhasorMode_EnvType;
      break;
    case SynthMode::Pan_PanRnd:
      break;
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

bool UIManager::EncoderLongPress(){
  return pod_.encoder.TimeHeldMs() > 1000.0f;
}

bool UIManager::Button1Pressed(){
  return pod_.button1.FallingEdge();
}

bool UIManager::Button2Pressed(){
  return pod_.button2.FallingEdge();
}

bool UIManager::Button2LongPress(){
  return pod_.button2.TimeHeldMs() > 1000.0f;
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