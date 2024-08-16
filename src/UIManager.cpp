#include "UIManager.h"

/// @brief Initialise UI, set up timer for LED pulse callbacks
void UIManager::Init(){
  SetupTimer();
  // StartLedPulse();
}

/// @brief Called on loop to continually update knob inputs and current app state
void UIManager::UpdateUI(){
  if (!crash_error){
    UpdateKnobs();
      
    UpdateState();
    // UpdateSynthMode();
  }
}

/// @brief Updates hardware input knob values and assigns to correct array indices 
void UIManager::UpdateKnobs(){
  // NOTE: need to add stuff for chord mode here! 
  if (current_state_ == AppState::Synthesis){
    int mode_idx = static_cast<int>(synth_mode_);
    float k1v = MapKnobDeadzone(pod_.knob1.Process());
    float k2v = MapKnobDeadzone(pod_.knob2.Process());

    /* here we pass the current knob value, pointer to the stored knob value for this mode,
      and a pointer to the bool which tracks whether the knob value has been passed through */
    k1v_[mode_idx] = UpdateKnobPassThru(k1v, &k1v_[mode_idx], &k1_pass_thru_[mode_idx]);
    k2v_[mode_idx] = UpdateKnobPassThru(k2v, &k2v_[mode_idx], &k2_pass_thru_[mode_idx]);
  }
}

/// @brief Updates current UI/app state based on hardware inputs
void UIManager::UpdateState(){
  pod_.ProcessDigitalControls();
  // TODO: change this to encoder pressed?? 
  if (Button1Pressed()){
    switch(current_state_){
      case AppState::Startup:
        current_state_ = AppState::SelectFile; 
        break;
      case AppState::SelectFile:
        current_state_ = AppState::PlayWAV;
        // NOTE: should i select next state (ie playwav) by clicking encoder? or pressing button1? 
        break;
      case AppState::PlayWAV:
        current_state_ = AppState::Synthesis;
        synth_mode_ = SynthMode::Size_Position;
        // StopLedPulse(); 
        break;
      case AppState::Synthesis:
        UpdateSynthMode();
        break;
      default:
        break;
    }
  }
  if (Button2Pressed()){
    switch(current_state_){
      case AppState::Synthesis:
        ToggleRandomnessControls();
        SetLedRandomMode();
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
        // StartLedPulse();
        break;
      case AppState::SelectFile:
        current_state_ = AppState::RecordIn;
        break;
      default:
        break;
    }
  }
  if (EncoderPressed()){
    if (current_state_ == AppState::RecordIn){
      current_state_ = AppState::Synthesis;
    }
  }
}

/// @brief Helper method to externally set current app state
/// @param state 
void UIManager::SetState(AppState state){
  current_state_ = state;
}

/// @brief Helper method to set current app state to Error and start red error LEDs
void UIManager::SetStateError(){
  current_state_ = AppState::Error;
  StopLedPulse();
  SetLed(255,0,0,true);
  SetLed(255,0,0,false);
  crash_error = true;
}

/// @brief Cycles through synth parameter update modes
void UIManager::UpdateSynthMode(){
  switch(synth_mode_){
    case SynthMode::Size_Position:
      synth_mode_ = SynthMode::Pitch_ActiveGrains;
      break;
    case SynthMode::Pitch_ActiveGrains:
      synth_mode_ = SynthMode::PhasorMode_EnvType;
      break;
    case SynthMode::PhasorMode_EnvType:
      synth_mode_ = SynthMode::Pan_PanRnd;
      break;
    case SynthMode::Pan_PanRnd:
      synth_mode_ = SynthMode::Size_Position;
      break;
    default:
      break;
  }
}

/* each synth mode (apart from pan) has 2 submodes: 
  knobs either change the named parameters themselves (modes above), 
  or change the degree of randomness applied to these parameters in granulation */
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


bool UIManager::ToggleRecordOut(){//NOTE NEED TO DECIDE THiS!!
  // TODO: need to decide what control input will toggle record out
  return true; 
}


float UIManager::GetKnob1Value(int mode_idx){
  return k1v_[mode_idx];
}

float UIManager::GetKnob2Value(int mode_idx){
  return k2v_[mode_idx];
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

/* has to be static - timer won't take class member function in callback  */
static void StaticLedCallback(void* data) {
  UIManager* instance = static_cast<UIManager*>(data);
  instance->LedPulseCallback();
}


// TODO
// NOTE: COULD REFACTOR THIS INTO A SEPARATE CLASS ?? 
// but then have to pass to this class..
void UIManager::SetupTimer(){
  TimerHandle::Config cfg;
  cfg.periph = TimerHandle::Config::Peripheral::TIM_5;
  // /* set period for 200ms pulse (1MHz/200,000 == 200ms) */
  // cfg.period = 200000;
  /* set period for 10ms callback interval */
  cfg.period = 1000000;
  cfg.enable_irq = true;
  timer_.Init(cfg);
  /* set 1Mhz tick frequency */
  timer_.SetPrescaler(199);
  pulse_increasing_ = true;
  timer_.SetCallback(StaticLedCallback, this);
}



void UIManager::LedPulseCallback(){
  if (pulse_increasing_){
    /* NOTE: increase step (in header) and period to reduce processing cost */
    pulse_brightness_ += PULSE_STEP;
    if (pulse_brightness_ >= 255 - PULSE_STEP){
      pulse_brightness_ = 255;
      pulse_increasing_ = false;
    }
  }
  else {
    pulse_brightness_ -= PULSE_STEP;
    if (pulse_brightness_ <= PULSE_STEP){
      pulse_count_++;
      pulse_brightness_ = 0;
      pulse_increasing_ = true;
    }
  }
  switch(current_state_){
    /* pulse blue */
    case AppState::SelectFile:
      SetLed(0,0, pulse_brightness_, true);
    /* pulse white */
    case AppState::RecordIn:
      SetLed(pulse_brightness_, pulse_brightness_, pulse_brightness_, true);
    /* pulse cyan */
    case AppState::PlayWAV:
      SetLed(0, pulse_brightness_, pulse_brightness_, true);
    /* pulse red */
    case AppState::Error:
      SetLed(pulse_brightness_,0,0,true);

    /* when switching to synth mode, pulse green twice, then solid;
        otherwise pulses on synth mode switch */
    case AppState::Synthesis:
      if (pulse_count_ <= 2){
        SetLed(0, pulse_brightness_, 0, true);
      }
      else {
        StopLedPulse();
        SetLedSynthMode();
      }
      break;
    default:
      break;
  }
} 

void UIManager::SetLedSynthMode(){
  switch(synth_mode_){
    case SynthMode::Size_Position:
      SetLed(0,255,0,true);
      break;
    case SynthMode::Pitch_ActiveGrains:
    // orange: 100, 64, 0 or 255, 165, 0 or 255, 91, 31 (neon orange)
      BlinkSetLed(255, 91, 31,true);
      break;
    case SynthMode::PhasorMode_EnvType:
    // 0, 0, 255 blue
      BlinkSetLed(0,0,255,true);
      break;
    case SynthMode::Pan_PanRnd:
    // fuschia: 255,0,255
      BlinkSetLed(255,0,255,true);
      break;
    default:
      break;
  }
}

void UIManager::SetLedRandomMode(){
  switch(synth_mode_){
    case SynthMode::Size_Position:
    case SynthMode::Pitch_ActiveGrains:
    case SynthMode::PhasorMode_EnvType:
    case SynthMode::Pan_PanRnd:
      /* set led 2 to green*/
      SetLed(0,255,0,false);
      break;
    
    case SynthMode::Size_Position_Rnd:
    case SynthMode::Pitch_ActiveGrains_Rnd:
    case SynthMode::PhasorMode_EnvType_Rnd:
      /* set led 2 to red*/
      SetLed(255,0,0,false);
      break;
  }
}

void UIManager::StartLedPulse(){
  pulse_brightness_ = 0;
  pulse_increasing_ = true;
  pulse_count_ = 0;
  timer_.Start();
}

void UIManager::StopLedPulse(){
  timer_.Stop();
}

void UIManager::SetLed(int r, int g, int b, bool is_Led1){
  if (is_Led1){
    pod_.led1.Set(r,g,b);
  }
  else {
    pod_.led2.Set(r,g,b);
  }
  pod_.UpdateLeds();
  System::Delay(10);
}

void UIManager::BlinkLed(int r, int g, int b, bool is_Led1){
  SetLed(r,g,b, is_Led1);
  System::Delay(100);
  SetLed(0,0,0, is_Led1);
}

void UIManager::BlinkSetLed(int r, int g, int b, bool is_Led1){
  BlinkLed(r,g,b, is_Led1);
  System::Delay(100);
  BlinkLed(r,g,b, is_Led1);
  System::Delay(100);
  SetLed(r,g,b, is_Led1);
}
