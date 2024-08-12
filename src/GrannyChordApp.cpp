#include "GrannyChordApp.h"

void GrannyChordApp::Init(){
  // 
  pod_.SetAudioBlockSize(4);
  pod_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  ui_.Init();
  if (!InitFileMgr()){
    return;
  }
  // start audio callback in main function? 
  // since i'll create an audiocallback fn in this class
  // so just pass it to main?
  // or start it here..? 
  if (!HandleFileSelection()){
    return;
  }
  
  
  
  
  
  InitCompressor();
}

void GrannyChordApp::Run(){
  // update ui state, handle controls, playback, synthesis, chords etc
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
}

bool GrannyChordApp::CheckParamDelta(float curr_val, float prev_val){
  return (fabsf(curr_val - prev_val)>0.01f);
}