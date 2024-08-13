#include "GrannyChordApp.h"

void GrannyChordApp::Init(){
  // 
  pod_.SetAudioBlockSize(4);
  pod_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  ui_.Init();
  if (!InitFileMgr()){
    return;
  }
  if (!HandleFileSelection()){
    return;
  }
  InitCompressor();

  // init synth? or do that in main? 
  InitPrevParamVals();



  // pod_.StartAdc();
  // pod_.StartAudio(AudioCallback);
}

// static void AudioCallback(){

// }

void GrannyChordApp::Run(){
  while(true){
    ui_.UpdateControls();
    UpdateSynthParams();
    // handle states, playback, chords
    System::Delay(1);
  }
}

void GrannyChordApp::UpdateSynthParams(){
  AppState curr_state = ui_.GetCurrentState();
  if (curr_state==AppState::Synthesis){
    UpdateGranularParams();
  }
  // else if (curr_state==AppState::ChordMode){
  //   UpdateChordParams();
  // }
}

void GrannyChordApp::UpdateGranularParams(){
  SynthMode mode = ui_.GetSynthMode();
  int mode_idx = static_cast<int>(mode);
  float knob1_val = ui_.GetKnob1Value(mode_idx);
  float knob2_val = ui_.GetKnob2Value(mode_idx);
  if (CheckParamDelta(knob1_val, prev_param_vals_k1[mode_idx])){
    switch (mode){
      case SynthMode::Size_Position:
        synth_.SetUserGrainSize(knob1_val);
        break;
      case SynthMode::Pitch_ActiveGrains:
        synth_.SetUserPitchRatio(knob1_val);
        break;
      case SynthMode::Pan_PanRnd:
        synth_.SetPan(knob1_val);
        break;
      case SynthMode::PhasorMode_EnvType:
        knob1_val = fmap(knob1_val, 0, NUM_PHASOR_MODES);
        synth_.SetPhasorMode(static_cast<GrainPhasor::Mode>(knob1_val));
        break;
      case SynthMode::Size_Position_Rnd:
        synth_.SetSizeRnd(knob1_val);
        break;
      case SynthMode::Pitch_ActiveGrains_Rnd:
        synth_.SetPitchRnd(knob1_val);
        break;
      case SynthMode::PhasorMode_EnvType_Rnd:
        synth_.SetPhasorRnd(knob1_val);
        break;
    }
  prev_param_vals_k1[mode_idx] = knob1_val;
  }
  if (CheckParamDelta(knob2_val, prev_param_vals_k2[mode_idx])){
    switch (mode){
      case SynthMode::Size_Position:
        synth_.SetUserSpawnPos(knob2_val);
      case SynthMode::Pitch_ActiveGrains:
        synth_.SetUserActiveGrains(knob2_val);
      case SynthMode::Pan_PanRnd:
        synth_.SetPanRnd(knob2_val);
        break;
      case SynthMode::PhasorMode_EnvType:
        knob2_val = fmap(knob2_val, 0, NUM_ENV_TYPES);
        synth_.SetEnvelopeType(static_cast<Grain::EnvelopeType>(knob2_val));
        break;
      case SynthMode::Size_Position_Rnd:
        synth_.SetPositionRnd(knob2_val);
        break;
      case SynthMode::Pitch_ActiveGrains_Rnd:
        synth_.SetCountRnd(knob2_val);
        break;
      case SynthMode::PhasorMode_EnvType_Rnd:
        synth_.SetEnvRnd(knob2_val);
        break;
    }
    prev_param_vals_k2[mode_idx] = knob2_val;
  }
}


bool GrannyChordApp::InitFileMgr(){
  if (!filemgr_.Init()){
      ui_.SetStateError();
      return false;
  }
  if (!filemgr_.ScanWavFiles()){
    ui_.SetStateError();
    return false;
  }
  if (filemgr_.LoadFile(0)){
    InitSynth();
  }
  else {
    ui_.SetStateError();
    return false;
  }
  return true;
}

void GrannyChordApp::InitCompressor(){
  comp.Init(pod_.AudioSampleRate());
  comp.SetRatio(3.0f);
  comp.SetAttack(0.01f);
  comp.SetRelease(0.1f);
  comp.SetThreshold(-12.0f);
  comp.AutoMakeup(true);
}

bool GrannyChordApp::HandleFileSelection(){
  if (ui_.GetCurrentState() != AppState::SelectFile){
    return false;
  }
  int32_t inc = ui_.GetEncoderIncrement();
  uint16_t file_count = filemgr_.GetFileCount();
  if (inc!=0){
    // char fname[64];
    file_idx_ += inc;
    if (file_idx_<0){ 
      file_idx_ = file_count - 1; 
    }
    if (file_idx_ >= file_count){ 
      file_idx_ = 0; 
    }
    
    // filemgr_.GetName(file_idx_, fname);
    // pod_.seed.PrintLine("selected new file idx %d %s", file_idx_, fname);
  }
  if (ui_.Button1Pressed()){
    if (filemgr_.LoadFile(file_idx_)){
      InitSynth();
      return true;
    } 
    else {
      ui_.SetStateError();
      // pod.seed.PrintLine("Failed to load audio file");
      return false;
    }
  }
  return true;
}

bool GrannyChordApp::CheckParamDelta(float curr_val, float prev_val){
  return (fabsf(curr_val - prev_val)>0.01f);
}