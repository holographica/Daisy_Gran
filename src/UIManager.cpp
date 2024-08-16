#include "UIManager.h"

/// @brief Initialise UI, set up timer for LED pulse callbacks
void UIManager::Init(){
  SetupTimer();
  SetState(AppState::Startup);
  StartLedPulse();
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
    int mode_idx = static_cast<int>(current_synth_mode_);
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
    HandleButton1();
  }
  if (Button2Pressed()){
    HandleButton2();
  }
  if (Button1LongPress()){
    HandleButton1LongPress();
  }
  if (Button2LongPress()){
    HandleButton2LongPress();
  }
  if (EncoderPressed()){
    HandleEncoderPressed();
  }
  if (EncoderLongPress()){
    HandleEncoderLongPress();
  }
  if (GetEncoderIncrement()>0){
    if (current_state_ == AppState::Startup){
      current_state_ = AppState::SelectFile;
    }
  }
}

void UIManager::HandleButton1(){
  switch(current_state_){
    case AppState::Startup:
      current_state_ = AppState::SelectFile; 
      break;
    // case AppState::SelectFile:
    //   current_state_ = AppState::PlayWAV;
    //   // NOTE: should i select next state (ie playwav) by clicking encoder? or pressing button1? 
    //   break;
    case AppState::PlayWAV:
      current_state_ = AppState::Synthesis;
      current_synth_mode_ = SynthMode::Size_Position;
      // StopLedPulse(); 
      break;
    case AppState::Synthesis:
      UpdateSynthMode();
      DebugPrintSynthMode();
      break;
    default:
      break;
  }
}

void UIManager::HandleButton2(){
    if (current_state_ == AppState::Synthesis){
      ToggleRandomnessControls();
      SetLedRandomMode();
    }
}

void UIManager::HandleButton1LongPress(){
  if (current_state_ == AppState::Synthesis){
    ToggleReverbControls();
    SetLedFXMode();
  }
}

void UIManager::HandleButton2LongPress(){
  if (current_state_ == AppState::Synthesis){
    ToggleFilterControls();
    SetLedFXMode();
  }
}

void UIManager::HandleEncoderPressed(){
  switch(current_state_){
    case AppState::SelectFile:
      current_state_ = AppState::PlayWAV;
    case AppState::RecordIn:
      current_state_ = AppState::PlayWAV;
    case AppState::PlayWAV:
      current_state_ = AppState::Synthesis;
      break;
    default:
      break;
  }
}

void UIManager::HandleEncoderLongPress(){
  switch (current_state_){
     /* exit synthesis mode, return to file selection */
    case AppState::Synthesis:
      current_state_ = AppState::SelectFile;
      // StartLedPulse();
      break;
      /* enter RecordIn mode */
    case AppState::SelectFile:
    case AppState::PlayWAV:
      current_state_ = AppState::RecordIn;
      break;
    case AppState::RecordIn:
      current_state_ = AppState::SelectFile;
      break;
    default:
      break;
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
  switch(current_synth_mode_){
    case SynthMode::Size_Position:
      current_synth_mode_ = SynthMode::Pitch_ActiveGrains;
      break;
    case SynthMode::Pitch_ActiveGrains:
      current_synth_mode_ = SynthMode::PhasorMode_EnvType;
      break;
    case SynthMode::PhasorMode_EnvType:
      current_synth_mode_ = SynthMode::Pan_PanRnd;
      break;
    case SynthMode::Pan_PanRnd:
      current_synth_mode_ = SynthMode::Size_Position;
      break;
    default:
      break;
  }
}

/* each synth mode (apart from pan) has 2 submodes: 
  knobs either change the named parameters themselves (modes above), 
  or change the degree of randomness applied to these parameters in granulation */
void UIManager::ToggleRandomnessControls(){
  switch(current_synth_mode_){
    case SynthMode::Size_Position:
      current_synth_mode_ = SynthMode::Size_Position_Rnd;
      break;
    case SynthMode::Size_Position_Rnd:
      current_synth_mode_ = SynthMode::Size_Position;
      break;
    case SynthMode::Pitch_ActiveGrains:
      current_synth_mode_ = SynthMode::Pitch_ActiveGrains_Rnd;
      break;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      current_synth_mode_ = SynthMode::Pitch_ActiveGrains;
      break;
    case SynthMode::PhasorMode_EnvType:
      current_synth_mode_ = SynthMode::PhasorMode_EnvType_Rnd;
      break;      
    case SynthMode::PhasorMode_EnvType_Rnd:
      current_synth_mode_ = SynthMode::PhasorMode_EnvType;
      break;
    default:
      break;
  }
}

void UIManager::ToggleReverbControls(){
  if (current_synth_mode_==SynthMode::Reverb){
    current_synth_mode_ = prev_synth_mode_;
  }
  else {
    prev_synth_mode_ = current_synth_mode_;
    current_synth_mode_ = SynthMode::Reverb;
  }
}

void UIManager::ToggleFilterControls(){
  if (current_synth_mode_==SynthMode::Filter){
    current_synth_mode_ = prev_synth_mode_;
  }
  else {
    prev_synth_mode_ = current_synth_mode_;
    current_synth_mode_ = SynthMode::Filter;
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

bool UIManager::Button1LongPress(){
  return pod_.button1.TimeHeldMs() > 1000.0f;
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
  instance->LedCallback();
}

void UIManager::SetupTimer(){
  TimerHandle::Config cfg;
  cfg.periph = TimerHandle::Config::Peripheral::TIM_5;
  cfg.period = 10000;
  cfg.enable_irq = true;
  timer_.Init(cfg);
  timer_.SetPrescaler(9999);
  timer_.SetCallback(StaticLedCallback, this);
}


void UIManager::LedCallback(){
  switch(current_state_){

  }
}


/* 
SelectFile: BLUE
RecordIn: WHITE
PlayWAV: CYAN
Synthesis: GREEN
ChordMode: GOLD (orange)

Error: solid RED
RecordIn: seed led flashing red (ie pod.seed.SetLed(1/0)

don't use purple - looks like blue
can use magenta (255,0,255)
can use yellow (255,255,0)
*/

/*
#define OFF	0, 0, 0
#define RED 	1, 0, 0
#define GREEN	0, 1, 0
#define BLUE	0, 0, 1
#define LBLUE	0, 0.7f, 1
#define LGREEN	0, 1, 0.7f
#define WHITE   0.7f, 0.7f, 0.7f  
#define YELLOW  0.7f, 0.7f, 0  
#define ORANGE  1, 0.7f, 0  
#define ROSE    1, 0, 0.7f
#define VIOLET  0.7f, 0, 1
#define PURPLE  0.7f, 0, 0.7f  
#define CYAN	0, 0.7f, 0.7f  

*/



void UIManager::LedCallback(){
  switch(current_state_){
    /* pulse blue */
    case AppState::SelectFile:
      SetLed(0, 0, led_brightness_, true);
    /* pulse white */
    case AppState::RecordIn:
      SetLed(led_brightness_, led_brightness_, led_brightness_, true);
    /* pulse cyan */
    case AppState::PlayWAV:
      SetLed(0, led_brightness_, led_brightness_, true);
    /* pulse red */
    case AppState::Error:
      SetLed(led_brightness_,0, 0, true);

    /* when switching to synth mode, pulse green twice, then solid;
        otherwise pulses on synth mode switch */
    case AppState::Synthesis:
        SetLed(0, led_brightness_, 0, true);
        StopLedPulse();
        SetLedSynthMode();
      break;
    default:
      break;
  }
} 

void UIManager::SetLedSynthMode(){
  if (current_state_ == AppState::Synthesis){
    switch(current_synth_mode_){
      case SynthMode::Size_Position:
        pod_.led1.SetColor();
        break;
      case SynthMode::Pitch_ActiveGrains:
      // orange: 100, 64, 0 or 255, 165, 0 or 255, 91, 31 (neon orange)
        pod_.led1.Set(255,91,31);
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
}

void UIManager::SetLedRandomMode(){
  if (current_state_==AppState::Synthesis){
    switch(current_synth_mode_){
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
      default:
        break;
    }
  }
}

void UIManager::SetLedFXMode(){
  if (current_state_ == AppState::Synthesis){
    switch(current_synth_mode_){
      case SynthMode::Reverb:
        /* set led 2 to cyan */
        SetLed(0, 255, 255, false);
        break;
      case SynthMode::Filter:
        /* set led 2 to yellow */
        SetLed(255, 255, 0, false);
        break;
      default:
        break;
    }
  }
}

void UIManager::StartLedPulse(){
  led_brightness_ = 0;
  timer_.Start();
}

void UIManager::StopLedPulse(){
  timer_.Stop();
}


void UIManager::DebugPrintState(){
  switch(current_state_){
    case AppState::Startup:
      DebugPrint(pod_, "State now in: Startup");
        break;
    case AppState::SelectFile:
      DebugPrint(pod_, "State now in: SelectFile");
        break;
    case AppState::RecordIn:
      DebugPrint(pod_, "State now in: RecordIn");
        break;
    case AppState::PlayWAV:
      DebugPrint(pod_, "State now in: PlayWAV");
        break;
    case AppState::Synthesis:
      DebugPrint(pod_, "State now in: Synthesis");
        break;
    // case AppState::ChordMode:
      // DebugPrint(pod_, "State now in: ChordMode");
    case AppState::Error:
      DebugPrint(pod_, "State now in: Error");
        break;
    default:
      DebugPrint(pod_, "default state??");
      break;
  }
}

void UIManager::DebugPrintSynthMode(){
  switch(current_synth_mode_){
    case SynthMode::Size_Position:
      DebugPrint(pod_, "Synth mode now in: Size_Position");
        break;
    case SynthMode::Size_Position_Rnd:
      DebugPrint(pod_, "Synth mode now in: Size_Position_Rnd");
        break;
    case SynthMode::Pitch_ActiveGrains:
      DebugPrint(pod_, "Synth mode now in: Pitch_ActiveGrains");
        break;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      DebugPrint(pod_, "Synth mode now in: Pitch_ActiveGrains_Rnd");
        break;
    case SynthMode::PhasorMode_EnvType:
      DebugPrint(pod_, "Synth mode now in: PhasorMode_EnvType");
        break;
    case SynthMode::PhasorMode_EnvType_Rnd:
      DebugPrint(pod_, "Synth mode now in: PhasorMode_EnvType_Rnd");
        break;
    case SynthMode::Pan_PanRnd:
      DebugPrint(pod_, "Synth mode now in: Pan_PanRnd");
      break;
    case SynthMode::Reverb:
      DebugPrint(pod_, "Synth mode now in: Reverb");
      break;
    case SynthMode::Filter:
      DebugPrint(pod_, "Synth mode now in: Filter");
      break;
  }
}

