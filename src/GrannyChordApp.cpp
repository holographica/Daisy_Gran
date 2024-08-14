#include "GrannyChordApp.h"

void GrannyChordApp::Init(int16_t *left, int16_t *right){
  left_buf_ = left;
  right_buf_ = right;
  pod_.SetAudioBlockSize(4);
  pod_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  SeedRng();
  ui_.Init();
  if (!InitFileMgr()){
    return;
  }
  ui_.SetState(AppState::SelectFile);
  InitFileSelection();
  InitCompressor();
  InitPrevParamVals();

  pod_.StartAdc();
  pod_.StartAudio(AudioCallback);
}

void GrannyChordApp::Run(){
  while(true){
    AppState prev_state = ui_.GetCurrentState();
    ui_.UpdateUI();
    AppState curr_state = ui_.GetCurrentState();

    if (prev_state!=curr_state){
      HandleStateChange(prev_state, curr_state);
    }
    UpdateSynthParams(curr_state);

    if (ui_.ToggleRecordOut()){
      if (recording_out_) { StopRecordOut(); }
      else { StartRecordOut(); }
    }
    System::Delay(1);
  }
}

void GrannyChordApp::AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  AppState curr_state = instance_->ui_.GetCurrentState();
  // if in RecordIn: clear channel buffers, send input straight to them
  // track audio length, if greater than (60*48000) ie 60s, wrap index
  // to front of buffer so it's circular and can't record > 60s
  // then set audio_len_ vbl to tracked length so synth works properly
  switch (curr_state){
    case AppState::PlayWAV:
      break;
    case AppState::RecordIn:
      instance_->ProcessRecordIn(in, out, size);
      break;
    case AppState::Synthesis:
      instance_->ProcessSynthesis(out,size);
      break;
    // case AppState::ChordMode:
    // case AppState:RecordOut:
    default:
      instance_->AudioIdle(out,size);
      break;
  }
}

void GrannyChordApp::AudioIdle(AudioHandle::OutputBuffer out, size_t size){
  for (size_t i=0; i<size; i++){
    out[0][i]=out[1][i]=0.0f;
  }
}

void GrannyChordApp::ProcessWAVPlayback(AudioHandle::OutputBuffer out, size_t size){
  if (wav_playhead_>=audio_len_){
    wav_playhead_ = 0;
  }
  for (size_t i=0; i<size; i++){
    if (wav_playhead_ < audio_len_){
      out[0][i] = s162f(left_buf_[wav_playhead_]);
      out[1][1] = s162f(right_buf_[wav_playhead_]);
      wav_playhead_++;
    }
    else {
      out[0][i]=out[1][i]=0.0f;
    }
  }
}

void GrannyChordApp::ProcessRecordIn(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  static const size_t MAX_RECORDING_LEN = 60*48000; // 60s @ 48kHZ
  static size_t record_pos = 0;
  for (size_t i=0; i<size;i++){
    /* send audio in straight to output for monitoring */
    out[0][i]=in[0][i];
    out[1][i]=in[1][i];

    /* record audio in to SDRAM buffers */
    left_buf_[record_pos] = in[0][i];
    right_buf_[record_pos] = in[1][i];

    /* wrap around recording length - if it exceeds 60s,
      the start of the recording will be overwritten */
    record_pos = (record_pos+1)%MAX_RECORDING_LEN;

  }
}

void GrannyChordApp::ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size){
  for (size_t i=0; i<size; i++){
    synth_.ProcessGrains(out[0],out[1],size);
    comp_.ProcessBlock(out[0],out[0],size);
    comp_.ProcessBlock(out[1],out[1],size);
  }
}

void GrannyChordApp::ProcessChordMode(AudioHandle::OutputBuffer out, size_t size){
  // if (UserTriggeredChord()) {
  //   synth_.TriggerChord(out[0], out[1], size);
  // } else {
  //     std::memset(out[0], 0, size * sizeof(float));
  //     std::memset(out[1], 0, size * sizeof(float));
  // }
}

void GrannyChordApp::RecordOutToSD(AudioHandle::OutputBuffer out, size_t size){
  // create file on sd card 
  // write in blocks
  // will have to f2s16 to conv back to int16 
  // max recording duration should be 120*48000*sizeof(float)*2(channels)??
  // wrap around index ? or just stop recording at max length
  // could flash seed led if wrapping, solid if stop at max
  // write straight to sd card?
  // or is it better to write to a circular buffer in SDRAM
  // then asynchronously/more slowly write to SD card
  // reset recording buffer pointer
  // set up audio input gain or anything?
  // init sample counter for recording duration? 
  // when finished: update file headers
  // update UI to show recording finished
}

void GrannyChordApp::StartRecordOut(){
  recording_out_=true;
}

void GrannyChordApp::StopRecordOut(){
  recording_out_ = false;
}

void GrannyChordApp::HandleStateChange(AppState prev, AppState curr){
  /* clean up from previous state */
  switch (prev){
    case AppState::SelectFile:

      break;
    case AppState::PlayWAV:
      
      // what do i need to do here?
      break;
    case AppState::RecordIn:
      // stop recording / writing to buffers
      // set playhead to start
      // 
      break;
    case AppState::Synthesis:
      // stop audio output/synthesis? 
      // if we go to either chord mode or select file, audio will stop anyway
      // anything else here? 
      break;
    // case AppState::ChordMode:
      // break;
    // case AppState::RecordOut:
      // finish writing to sd card, close file? 
      // break;
    default:
      break;
  }
  /* prepare for new state */
  switch(curr){
    case AppState::SelectFile:
      InitFileSelection();
      break;
    case AppState::PlayWAV:
      InitPlayback();
      break;
    case AppState::RecordIn:
      InitRecordIn();
      break;
    case AppState::Synthesis:
      InitSynth();
      break;
    // case AppState::ChordMode:
      // InitChordMode();
      // break;
    // case AppState::RecordOut:
      // do i need to do much here? 
      // don't stop audio since i'm recording
      // how do i handle this? need to stay in granular/chord mode
      // or at least keep outputting audio
      // should i just not make this a state? 
      // need to check sd card has memory, create file,
      // write to it until max recording length, close file,
      // return to previous state 
    case AppState::Error:
      // Handle error state? what to do....
      break;
    default:
      // what here? restart ? 
      break;
  }
}

bool GrannyChordApp::InitFileMgr(){
  filemgr_.SetBuffers(left_buf_,right_buf_);
  if (!filemgr_.Init()){
      ui_.SetStateError();
      return false;
  }
  if (!filemgr_.ScanWavFiles()){
    ui_.SetStateError();
    return false;
  }
  return true;
}

void GrannyChordApp::InitSynth(){
  synth_.Init(left_buf_, right_buf_, audio_len_);
}

void GrannyChordApp::InitCompressor(){
  comp_.Init(pod_.AudioSampleRate());
  comp_.SetRatio(3.0f);
  comp_.SetAttack(0.01f);
  comp_.SetRelease(0.1f);
  comp_.SetThreshold(-12.0f);
  comp_.AutoMakeup(true);
}

void GrannyChordApp::InitFileSelection(){
  file_idx_ =0;
  memset(left_buf_, 0, sizeof(left_buf_));
  memset(right_buf_, 0, sizeof(right_buf_));
  if (!HandleFileSelection()){
    ui_.SetStateError();
  }
}

void GrannyChordApp::InitPlayback(){
  wav_playhead_ = 0;
  audio_len_ = filemgr_.GetSamplesPerChannel();
  float  total_secs = static_cast<float>(audio_len_)/48000.0f;
  int mins = static_cast<int>(total_secs/60.0f);
  int secs = static_cast<int>(total_secs)%60;
  pod_.seed.PrintLine("File loaded: %d:%d ", mins,secs);
}

void GrannyChordApp::InitRecordIn(){
  memset(left_buf_, 0, sizeof(left_buf_));
  memset(right_buf_, 0, sizeof(right_buf_));
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
    /* wrap around list of files */
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
      InitPlayback();
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

void GrannyChordApp::UpdateSynthParams(AppState curr_state){
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

bool GrannyChordApp::CheckParamDelta(float curr_val, float prev_val){
  return (fabsf(curr_val - prev_val)>0.01f);
}

void GrannyChordApp::InitPrevParamVals(){
      for (int i=0; i < NUM_SYNTH_MODES;i++){
        prev_param_vals_k1[i] = 0.5f;
        prev_param_vals_k2[i] = 0.5f;
      } 
};