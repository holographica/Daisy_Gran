#ifdef DEBUG_MODE // TODO: remove
#pragma message("Debug mode is ON")
#else
#pragma message("Debug mode is OFF")
#endif

#include "GrannyChordApp.h"

/* this is required for AudioCallback as it needs a static object */
GrannyChordApp* GrannyChordApp::instance_ = nullptr;


/// @brief Initialises app state and members and goes through app startup process
/// @param left Left channel audio data buffer
/// @param right Right channel audio data buffer
void GrannyChordApp::Init(int16_t *left, int16_t *right){
  left_buf_ = left, right_buf_ = right;
  pod_.SetAudioBlockSize(2);
  pod_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  curr_state_ = AppState::SelectFile;
  // SetupTimer();
  if (!InitFileMgr()){
    DebugPrint(pod_,"File manager failed to init");
    pod_.led1.SetRed(1);
    pod_.led2.SetRed(1);
    pod_.UpdateLeds();
    return;
  }
  loadmeter.Init(pod_.AudioSampleRate(), pod_.AudioBlockSize());
  InitFX();
  InitPrevParamVals();
  SeedRng();
  synth_.Init(left_buf_, right_buf_, 0);
  pod_.StartAdc();
}

#ifdef DEBUG_MODE
void GrannyChordApp::PrintCPULoad(){
  const float avgLoad = loadmeter.GetAvgCpuLoad();
  const float maxLoad = loadmeter.GetMaxCpuLoad();
  const float minLoad = loadmeter.GetMinCpuLoad();
  pod_.seed.PrintLine("Processing Load %:");
  pod_.seed.PrintLine("Max: " FLT_FMT3, FLT_VAR3(maxLoad * 100.0f));
  pod_.seed.PrintLine("Avg: " FLT_FMT3, FLT_VAR3(avgLoad * 100.0f));
  pod_.seed.PrintLine("Min: " FLT_FMT3, FLT_VAR3(minLoad * 100.0f));
}
#endif

/// @brief Loops whilst app is running, updating state and UI, handling 
///        state transitions, updating parameters and managing audio recordings
void GrannyChordApp::Run(){
  pod_.seed.PrintLine("app is running");
  while(true){
    // #ifdef DEBUG_MODE
    // loop_count++;
    // if (loop_count%2000000==0){
    //   PrintCPULoad();
    //   loop_count=0;
    //   // System::Delay(500);
    // }
    // #endif

    if (recording_out_) {
      sd_writer_.Write();
      if (sd_writer_.GetLengthSeconds()>120){
        sd_writer_.SaveFile();
        recording_out_ = false;
      }
    }
    UpdateUI();
    UpdateSynthParams();
    // System::Delay(1);
  }
}

/// @brief Handles hardware control inputs and calls state change methods
void GrannyChordApp::UpdateUI(){
  pod_.ProcessDigitalControls();

  if (pod_.encoder.TimeHeldMs() > 1000.0f) HandleEncoderLongPress();
  else if (pod_.encoder.FallingEdge()) HandleEncoderPressed();

  if (curr_state_==AppState::Synthesis){
    ButtonHandler();
  }

  int32_t encoder_inc = pod_.encoder.Increment();
  if (encoder_inc!=0){
    HandleEncoderIncrement(encoder_inc);
  }
  if (curr_state_ != next_state_){
    #ifdef DEBUG_MODE
    DebugPrintState(curr_state_);
    DebugPrint(pod_, "switching states");
    DebugPrintState(next_state_);
    #endif
    HandleStateChange();
  }
}

/// @brief handles transitions between states and prepares for next state
void GrannyChordApp::HandleStateChange(){
  switch(next_state_){
    case AppState::SelectFile:
      pod_.StopAudio();
      DebugPrint(pod_, "stopped audio for select file");
      curr_state_ = AppState::SelectFile;
      break; 
    case AppState::RecordIn:
      InitRecordIn();
      DebugPrint(pod_, "Recording in");
      curr_state_ = AppState::RecordIn;  
      break;
    case AppState::PlayWAV:
      InitPlayback(); 
      curr_state_ = AppState::PlayWAV;
      break;
    case AppState::Synthesis:
      InitSynth();
      curr_state_ = AppState::Synthesis;
      DebugPrint(pod_, "started audiocallback");
      break;
    case AppState::Error: 
      curr_state_ = AppState::Error;
      break;
    default: 
      break;
  }
  SetLed1AppState();
}

/// @brief Handles when encoder is scrolled to select a file 
/// @param encoder_inc amount the encoder has been scrolled/incremented
void GrannyChordApp::HandleEncoderIncrement(int encoder_inc){
  if (next_state_== AppState::SelectFile){
    recorded_in_ = false;
    HandleFileSelection(encoder_inc);
  }
  else if (curr_state_==AppState::PlayWAV){
    recorded_in_ = false;
    next_state_ = AppState::SelectFile;
  }
}

/// @brief handles when the encoder is pressed down (not held)
void GrannyChordApp::HandleEncoderPressed(){
  switch(curr_state_){
    case AppState::SelectFile:
      next_state_ = AppState::PlayWAV;
      return;
    case AppState::RecordIn: 
      recorded_in_ = true;
      next_state_ = AppState::PlayWAV; 
      return;
    case AppState::PlayWAV:
      next_state_ = AppState::Synthesis;
      return;
    default:
      return;
  }
}

/// @brief switch between states by long pressing encoder.
///        much simpler than below!
void GrannyChordApp::HandleEncoderLongPress(){
  do { pod_.encoder.Debounce();  }
  while(!pod_.encoder.FallingEdge());
  switch(curr_state_){
    case AppState::Synthesis:
    case AppState::RecordIn:  
      next_state_ = AppState::SelectFile;
      return;
    case AppState::SelectFile:
    case AppState::PlayWAV: 
      recorded_in_ = true;
      next_state_ = AppState::RecordIn;  
      return;
    default:
      return;
  }
}

/* this logic is required for button long presses:
  the button update rate is extremely fast so without
  a debounce delay, the method eg HandleButton1LongPress 
  will be called many 1000s of times per second 
  once time held > 1000ms, causing issues */
void GrannyChordApp::ButtonHandler(){
  unsigned long time_now = System::GetNow();
  /* only check button events if there's been a gap between them */
  if (time_now - last_action_time_ < DEBOUNCE_DELAY) {
    return;
  }

  /* store whether button 1 has been pressed down initiall*/
  if (pod_.button1.RisingEdge()){
    last_action_time_ = time_now;
    btn1_long_press_fired_ = false;
  }
  
  /* handle case when button 1 is held down */
  if (pod_.button1.Pressed()){
    float held_time = pod_.button1.TimeHeldMs();
    if (held_time > LONG_PRESS_TIME && !btn1_long_press_fired_){
      HandleButton1LongPress();
      btn1_long_press_fired_ = true;
      last_action_time_ = time_now;
    }
  }
  
  /* handle case when button 1 is released */
  if (pod_.button1.FallingEdge()){
    if (!btn1_long_press_fired_){
      HandleButton1();
    }
    last_action_time_ = time_now;
    btn1_long_press_fired_ = false;
  }

  /* store whether button 2 has been pressed down initially */
  if (pod_.button2.RisingEdge()){
    last_action_time_ = time_now;
    btn2_long_press_fired_ = false;
  }
  
  /* handle case when button 2 is held */
  if (pod_.button2.Pressed()){
    float held_time = pod_.button2.TimeHeldMs();
    if (held_time > LONG_PRESS_TIME && !btn2_long_press_fired_){
      HandleButton2LongPress();
      btn2_long_press_fired_ = true;
      last_action_time_ = time_now;
    }
  }
  
  /* handle case when button2 is released */
  if (pod_.button2.FallingEdge()){
    if (!btn2_long_press_fired_){
      HandleButton2();
    }
    last_action_time_ = time_now;
    btn2_long_press_fired_ = false;
  }

  /* handle case when both buttons are held down */
  if (pod_.button1.Pressed() && pod_.button2.Pressed()){
    float heldTime1 = pod_.button1.TimeHeldMs();
    float heldTime2 = pod_.button2.TimeHeldMs();
    if (heldTime1 > LONG_PRESS_TIME &&
        heldTime2 > LONG_PRESS_TIME &&
        !both_btns_long_press_fired_) 
    {
      if (!recording_out_){
        recording_out_ = true;
        DebugPrint(pod_,"now recording out!");
        RecordOutToSD();
      } 
      else{
        DebugPrint(pod_, "finished recording out: %.2fs",sd_writer_.GetLengthSeconds());
        FinishRecording();
      }
      both_btns_long_press_fired_ = true;
      last_action_time_ = time_now;
    }
  } 
  else{
    both_btns_long_press_fired_ = false;
  }
}

/// @brief Switch between synth parameter control modes
void GrannyChordApp::HandleButton1(){
  UpdateSynthModeA();
}

/// @brief Toggles on/off synth parameter randomness controls
void GrannyChordApp::HandleButton2(){
    UpdateSynthModeB();
}

/// @brief Toggles on/off reverb control mode 
void GrannyChordApp::HandleButton1LongPress(){
  // TODO: chord mode? 
}

/// @brief Toggles on/off filter control mode
void GrannyChordApp::HandleButton2LongPress(){
  // TODO: RECORD OUT? 
}

/// @brief Scrolls through list of files for user selection
void GrannyChordApp::HandleFileSelection(int32_t encoder_inc){
  int file_count = filemgr_.GetFileCount();
  file_idx_ = (file_idx_ + encoder_inc + file_count) % file_count;
  filemgr_.GetName(file_idx_,fname_);
  DebugPrint(pod_, "selected file %d %s",file_idx_,fname_);
}

/// @brief iterates through synth parameter control modes when button 1 pressed
/// if we are in an 'even' mode, go to the next 'eveb' mode. 
/// if we are in an 'odd' mode, go to the corresponding 'even' mode
void GrannyChordApp::UpdateSynthModeA(){
  DebugPrint(pod_, "next synth mode");
  int mode_idx = static_cast<int>(curr_synth_mode_);
  if (mode_idx%2 == 0){
    mode_idx = (mode_idx + 2) % 8;
  }
  else {
    /* XOR the enum value to get corresponding mode index */
    mode_idx = mode_idx ^ 1;
  }

  curr_synth_mode_ = static_cast<SynthMode>(mode_idx);
  DebugPrintMode(curr_synth_mode_);
  // ResetPassThru();
  SetLed1SynthMode();
  SetLed2();
  knob1_latched = true;
  knob2_latched = true;
}

/// @brief iterates through synth parameter control modes when button 2 pressed
/// if we are in an 'even' mode, go to the corresponding 'odd' mode. 
/// if we are in an 'odd' mode, go to the next 'odd' mode
void GrannyChordApp::UpdateSynthModeB(){
  DebugPrint(pod_, "next synth mode");
  int mode_idx = static_cast<int>(curr_synth_mode_);
  if (mode_idx %2 != 0){
    mode_idx = (mode_idx + 2) % 8;
  }
  else {
    mode_idx = mode_idx ^ 1;
  }

  curr_synth_mode_ = static_cast<SynthMode>(mode_idx);
  DebugPrintMode(curr_synth_mode_);
  // ResetPassThru();
  SetLed1SynthMode();
  SetLed2();
  knob1_latched = true;
  knob2_latched = true;
}

/// @brief Updates synth parameters based on current synth mode and adjusts
///        knob input values to account for knob jitter and deadzones around 0/1
void GrannyChordApp::UpdateSynthParams(){
  if (curr_state_!=AppState::Synthesis) return;
  int mode_idx = static_cast<int>(curr_synth_mode_);

  /* set to 0 or 1 if very close to these bounds */
  float knob1_val = MapKnobDeadzone(pod_.knob1.Process());
  float knob2_val = MapKnobDeadzone(pod_.knob2.Process());
  
  counter++;

  /* only update parameter if knob has passed through previous value in this mode */
  if (UpdateKnobPassThru(&knob1_latched, knob1_val,prev_param_k1[mode_idx])){
    UpdateKnob1Params(knob1_val,curr_synth_mode_);
    prev_param_k1[mode_idx] = knob1_val;
    if (counter%96000==0){
      DebugPrint(pod_, "new k1v: %f",knob1_val);
    }
  }

  if (UpdateKnobPassThru(&knob2_latched, knob2_val,prev_param_k2[mode_idx])){
    UpdateKnob2Params(knob2_val, curr_synth_mode_);
    prev_param_k2[mode_idx] = knob2_val;
    if (counter%96000==0){
      DebugPrint(pod_, "new k2v: %f",knob2_val);
    }
  }
}

/// @brief Initialises file manager, sets audio data buffers and scans SD card for WAV files
/// @return True on successful initialisation, false if init fails or no WAV files found
bool GrannyChordApp::InitFileMgr(){
  filemgr_.SetBuffers(left_buf_,right_buf_);
  if (!filemgr_.Init())return false;

  return filemgr_.ScanWavFiles();
}

/// @brief Calls synth initialisation function, passes audio data buffers and audio length
void GrannyChordApp::InitSynth(){
  synth_.Init(left_buf_, right_buf_, filemgr_.GetSamplesPerChannel());
  InitPrevParamVals();
  DebugPrint(pod_,"synth init ok - samples %u",filemgr_.GetSamplesPerChannel());
}

/// @brief Initialises WAV playback state, resets playhead, sets current file audio length
void GrannyChordApp::InitPlayback(){
  wav_playhead_ = 0;
  record_in_pos_  = 0;
  if (!recorded_in_){
    if (!filemgr_.LoadFile(file_idx_)) {
      DebugPrint(pod_,"failed to load file");
      curr_state_=AppState::Error;
      return;
    }
  }
  DebugPrint(pod_, "loaded file");
  pod_.StartAudio(AudioCallback);
}

/// @brief Initialises RecordIn state, clears audio buffers
void GrannyChordApp::InitRecordIn(){
  memset(left_buf_, 0, CHNL_BUF_SIZE_ABS);
  memset(right_buf_, 0, CHNL_BUF_SIZE_ABS);
  record_in_pos_ = 0;
}

// /// @brief Initialise object for recording out to SD card
void GrannyChordApp::InitWavWriter(){
  WavWriter<16384>::Config cfg;
  cfg.bitspersample = BIT_DEPTH;
  cfg.channels = filemgr_.GetNumChannels();
  cfg.samplerate = SAMPLE_RATE;
  sd_writer_.Init(cfg);
}

/// @brief initialise reverb, compressor, filter configs for FX section 
void GrannyChordApp::InitFX(){
  comp_.Init(SAMPLE_RATE_FLOAT);
  limiter_.Init();
  
  chorus_.Init(SAMPLE_RATE_FLOAT);
  reverb_.Init(SAMPLE_RATE_FLOAT);
  
  lowpass_moog_.Init(SAMPLE_RATE_FLOAT);
  lowpass_moog_.SetFreq(LOPASS_UPPER_BOUND);
  lowpass_moog_.SetRes(0.7f);
  
  hipass_.Init();
  hipass_.SetFilterMode(daisysp::OnePole::FilterMode::FILTER_MODE_HIGH_PASS);
  hipass_.SetFrequency(HIPASS_LOWER_BOUND);

  hicut_.SetFilterMode(daisysp::OnePole::FilterMode::FILTER_MODE_LOW_PASS);
  hicut_.SetFrequency(HICUT_FREQ);
}

/// @brief Initialise previous parameter value arrays to defaults
void GrannyChordApp::InitPrevParamVals(){
  for (int i=0; i < NUM_SYNTH_MODES;i++){
    prev_param_k1[i] = 0.5f;
    prev_param_k2[i] = 0.5f;
  } 
}

// void GrannyChordApp::ResetPassThru(){
//     int mode_idx = static_cast<int>(curr_synth_mode_);
//     stored_k1[mode_idx] = pod_.knob1.Process();
//     stored_k2[mode_idx] = pod_.knob2.Process();
//     pass_thru_k1 = false;
//     pass_thru_k2 = false;
// }

/// @brief Master audio callback, called at audio rate, calls all audio processing methods
/// @param in Audio input buffer
/// @param out Audio output buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  instance_->ProcessAudio(in,out,size);
}

void GrannyChordApp::ProcessAudio(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  switch(curr_state_){
    case AppState::PlayWAV:
      if (wav_playhead_>= filemgr_.GetSamplesPerChannel() -1){
        instance_->pod_.StopAudio();
        DebugPrint(pod_, "stopped audio > len"); 
        return;
      }
      ProcessWAVPlayback(out,size);
      // ProcessSynthesis(out,size);
      return;  
    case AppState::RecordIn:
      ProcessRecordIn(in, out, size);
      return;
    case AppState::Synthesis:
      ProcessSynthesis(out,size);
      return;
    // case AppState::ChordMode:
    // return;
    default:
      return;
  }
}

/// @brief Process audio for WAV playback mode and increment playback position
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::ProcessWAVPlayback(AudioHandle::OutputBuffer out, size_t size){
  for (size_t i=0; i<size; i++){
    counter++;
    if (wav_playhead_ < filemgr_.GetSamplesPerChannel()){
      out[0][i] = s162f(left_buf_[wav_playhead_]) * 0.5f;
      out[1][i] = s162f(right_buf_[wav_playhead_]) * 0.5f;
      wav_playhead_++;
    }
    if (counter%128000==0){
      counter=0;
      DebugPrint(pod_, "l: %f, r: %f", out[0][i],out[1][i]);
    }
  }
}

/// @brief Process input audio when app is in RecordIn state
/// @param in Input audio buffer
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::ProcessRecordIn(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  const size_t MAX_RECORDING_LEN = 120*48000; /* 120s @ 48kHZ */
  for (size_t i=0; i<size;i++){
    /* send audio in straight to output for monitoring */
    out[0][i]=in[0][i];
    out[1][i]=in[1][i];
    /* record audio in to SDRAM buffers */
    left_buf_[record_in_pos_] = in[0][i];
    right_buf_[record_in_pos_] = in[1][i];
    /* wrap around recording length - if it exceeds 120s,
      the start of the recording will be overwritten */
    record_in_pos_ = (record_in_pos_+1)%MAX_RECORDING_LEN;
  }
}

/// @brief Process audio through granular synth and mix to output buffer
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size){
  // Sample samp;
  for (size_t i=0; i<size; i++){ 
    Sample samp = synth_.ProcessGrains();
    counter++;

    // samp = ProcessFX(samp);
    // out[0][i] = samp.left;
    // out[1][i] = samp.right;
    limiter_.ProcessBlock(&samp.left, 1, 1.0f);
    limiter_.ProcessBlock(&samp.right, 1, 1.0f);
    if (counter%128000==0){
      counter=0;
      DebugPrint(pod_, "l: %f, r: %f", samp.left, samp.right);
    }
    out[0][i] = samp.left;
    out[1][i] = samp.right;

    //NOTE: TURN BACK ON 
    // if (recording_out_ && sd_writer_.GetLengthSeconds()<MAX_REC_OUT_LEN){
    //   temp_interleaved_buf_[0]=out[0][i];
    //   temp_interleaved_buf_[1]=out[1][i];
    //   sd_writer_.Sample(temp_interleaved_buf_);
    // }
  } 
}

Sample GrannyChordApp::ProcessFX(Sample in){
  Sample out;
  /* hipass: remove rumble */
  out.left = hipass_.Process(in.left);
  out.right = hipass_.Process(in.right);

  // /* apply compression to reduce gain changes */
  // out.left = comp_.Process(out.left);
  // out.right = comp_.Process(out.right);

  /* apply lowpass filter */
  out.left = lowpass_moog_.Process(out.left);
  out.right = lowpass_moog_.Process(out.right);

  /* apply stereo rotation */
  // out = rotator_.Process(out);

  /* apply chorus */
  // out.left = chorus_.Process(out.left);
  // out.right = chorus_.Process(out.right);

  /* apply reverb */
  // reverb_.ProcessMix(out.left, out.right, &out.left, &out.right);

  // out.left = hicut_.Process(out.left);
  // out.right = hicut_.Process(out.right);
  return out;
}

/// @brief Process audio from chord mode and mix to output buffer
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
// void GrannyChordApp::ProcessChordMode(AudioHandle::OutputBuffer out, size_t size){
//   // if (UserTriggeredChord()) {
//   //   synth_.TriggerChord(out[0], out[1], size);
//   // } else {
//   //     std::memset(out[0], 0, size * sizeof(float));
//   //     std::memset(out[1], 0, size * sizeof(float));
//   // }
// }

/// @brief Record granular synth or chord output audio to SD card
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::RecordOutToSD(){
  InitWavWriter();
  char name[32];
  sprintf(name,"recording_%d",recording_count_);
  recording_count_++;
  sd_writer_.OpenFile(name);
  recording_out_ = true;
}

/// @brief cleans up after recording to SD card is finished
void GrannyChordApp::FinishRecording(){
  sd_writer_.SaveFile();
  recording_out_=false;
}

/// @brief Updates synth parameters from hardware knob 1 input
/// @param knob1_val float between 0-1 from knob input
/// @param mode current synth mode that determines which parameters to update
void GrannyChordApp::UpdateKnob1Params(float knob1_val, SynthMode mode){
  switch (mode){
    case SynthMode::Size_Position:
      synth_.SetGrainSize(knob1_val);
      if (counter%128000==0){
        // DebugPrint(pod_, "set grain size to %u",synth_.GetSize());
      }
      return;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetPitchRatio(knob1_val);
      if (counter%128000==0){
        // DebugPrint(pod_, "set pitch to %f",synth_.GetPitch());
      }
      return;
    case SynthMode::Pan_Direction:
      // synth_.SetPan(knob1_val);
      return;
    case SynthMode::Reverb:
       /* set reverb feedback ie tail length */
      // reverb_.SetFeedback(knob1_val); // NOTE check it doesn't clip
      return;
    case SynthMode::Size_Position_Rnd:
      // synth_.SetSizeRnd(knob1_val);
      return;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      // synth_.SetPitchRnd(knob1_val);
      return;
    case SynthMode::PanRnd_Chorus:
      // rotator_.SetFreq(knob1_val*2.0f);
      return;     
    case SynthMode::Filter:
      /* map knob value to frequency range with an exponential curve */
      // knob1_val = fmap(knob1_val, LOPASS_LOWER_BOUND, LOPASS_UPPER_BOUND, daisysp::Mapping::EXP);
      /* set cutoff frequency of low pass moog filter */
      // lowpass_moog_.SetFreq(knob1_val);
      return;
  }
}

/// @brief Updates synth parameters from hardware knob 2 input
/// @param knob1_val float between 0-1 from knob input
/// @param mode current synth mode that determines which parameters to update
void GrannyChordApp::UpdateKnob2Params(float knob2_val, SynthMode mode){
  switch (mode){
    case SynthMode::Size_Position:
      synth_.SetSpawnPos(knob2_val);
      // if (counter%128000==0){
      //   // DebugPrint(pod_, "set pos to %u",synth_.GetPos());
      // }
      return;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetActiveGrains(knob2_val);
      // if (counter%128000==0){
      //   // DebugPrint(pod_, "set active grains to %u",synth_.GetCount());
      // }
      return;
    case SynthMode::Pan_Direction:
      synth_.SetDirection(knob2_val);
      return;
    case SynthMode::Reverb:
        // reverb_.SetMix(knob2_val);
        return;
    case SynthMode::Size_Position_Rnd:
      // synth_.SetPositionRnd(knob2_val);
      return;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      // synth_.SetCountRnd(knob2_val);
      return;
    case SynthMode::PanRnd_Chorus:
      /* set chorus left/right channel to (synth pan +/- 10%) */
      // chorus_.SetPan(fclamp(synth_.GetPan()+0.1f,0,1), (fclamp(synth_.GetPan()-0.1f,0,1)));
      // chorus_.SetLfoDepth(knob2_val);
      return;
    case SynthMode::Filter:
      /* map knob value to frequency range with linear curve */
      // knob2_val = fmap(knob2_val, HIPASS_LOWER_BOUND, HIPASS_UPPER_BOUND, daisysp::Mapping::EXP);
      /* set cutoff frequency for high pass filter */
      // hipass_.SetFrequency(knob2_val);
      return;
  }
}



/*

encoder for strum??? nice clicks 
knob1 pitch? 
knob2 position?
buttons chords? 
click encoder for granular stuff

*/

/* knobs on the Pod have deadzones around 0 and 1 so we adjust for this */
inline float GrannyChordApp::MapKnobDeadzone(float knob_val ){
  if (knob_val<=0.01f) return 0.0f;
  if (knob_val>=0.99f) return 1.0f;
  return knob_val;
}

inline bool GrannyChordApp::UpdateKnobPassThru(bool *knob_latched, float curr_knob_val, float prev_param){
  if ((*knob_latched)){
    if (fabs(curr_knob_val - prev_param)<0.0001){
      (*knob_latched) = false;
      return true;
    }
    else return false;
  }
  else return true;
}



void GrannyChordApp::DebugPrintState(AppState state){
  switch(state){
    case AppState::SelectFile:
      DebugPrint(pod_, "State now in: SelectFile");
      return;
    case AppState::RecordIn:
      DebugPrint(pod_, "State now in: RecordIn");
      return;
    case AppState::PlayWAV:
      DebugPrint(pod_, "State now in: PlayWAV");
      return;
    case AppState::Synthesis:
      DebugPrint(pod_, "State now in: Synthesis");
      return;
    // case AppState::ChordMode:
      DebugPrint(pod_, "State now in: ChordMode");
    case AppState::Error:
    default:
      DebugPrint(pod_, "State now in: Error");
      return;
  }
};

void GrannyChordApp::DebugPrintMode(SynthMode mode){
  switch(mode){
    case SynthMode::Size_Position:
      DebugPrint(pod_, "State now in: SizePos");
      return;
    case SynthMode::Size_Position_Rnd:
      DebugPrint(pod_, "State now in: SizePosRnd");
      return;
    case SynthMode::Pitch_ActiveGrains:
      DebugPrint(pod_, "State now in: PitchGrains");
      return;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      DebugPrint(pod_, "State now in: PitchGrainsRnd");
      return;
    case SynthMode::Pan_Direction:
      DebugPrint(pod_, "State now in: PanDirection");
      return;
    case SynthMode::PanRnd_Chorus:
      DebugPrint(pod_, "State now in: Rotator_Chorus");
      return;
    case SynthMode::Reverb:
      DebugPrint(pod_, "State now in: Reverb");
      return;
    case SynthMode::Filter:
      DebugPrint(pod_, "State now in: Filter");
      return;
  }
};

/// @brief sets up hardware timer peripheral, used for
///        timing led flashes on state/mode changes
///        (internal system delay is blocking so not used)
void GrannyChordApp::SetupTimer(){
  TimerHandle::Config cfg;
  cfg.periph = TimerHandle::Config::Peripheral::TIM_5;
  cfg.period = 4000;
  cfg.enable_irq = true;
  timer_.Init(cfg);
  timer_.SetPrescaler(9999);
  timer_.SetCallback(StaticLedCallback, nullptr);
  SetLed1AppState();
  timer_.Start();
}

/// @brief callback function called every timer period 
void GrannyChordApp::LedCallback(){
  if (led1_flash_count_ < 7) {
      /* flash on/off for the first 6 callbacks (3 cycles) */
      if (led1_flash_count_%2==0) { pod_.led1.Set(rgb1_[0],rgb1_[1],rgb1_[2]); }
      else { pod_.led1.Set(0,0,0);}
      led1_flash_count_++;
  }
  else pod_.led1.Set(rgb1_[0],rgb1_[1],rgb1_[2]); /* stay solid after 3 flashes */

  if (led2_flash_count_ <7){
    if (led2_flash_count_%2==0) { pod_.led2.Set(rgb2_[0],rgb2_[1],rgb2_[2]); }
    else { pod_.led2.Set(0,0,0); }
    led2_flash_count_++;
  } 
  else pod_.led2.Set(rgb2_[0],rgb2_[1],rgb2_[2]);

  if (recording_out_ || curr_state_ == AppState::RecordIn){
    pod_.seed.SetLed(seed_led_state_);
    seed_led_state_ = !seed_led_state_;
  }
  else pod_.seed.SetLed(0); /* turn off seed led when not recording out */
  pod_.UpdateLeds();
}

/// @brief helper function to set led1 colours
void GrannyChordApp::SetRgb1(float r, float g, float b){  
  rgb1_[0]= r;
  rgb1_[1]= g;
  rgb1_[2]= b;
}

/// @brief helper function to set led2 colours
void GrannyChordApp::SetRgb2(float r, float g, float b){
  rgb2_[0]= r;
  rgb2_[1]= g;
  rgb2_[2]= b;
}

/// @brief sets led colours based on app state
void GrannyChordApp::SetLed1AppState(){
  switch(curr_state_){
    case AppState::SelectFile:
      SetRgb1(0, 0, 0.7f); /* blue ? */
      break;
    case AppState::PlayWAV:
      SetRgb1(0, 0.7f, 0.7f); /* cyan */
      break;
    case AppState::Synthesis:
      SetRgb1(0, 0.7f, 0); /* green */
      break;
    case AppState::RecordIn:
      SetRgb1(0.7f,0.7f,0);  /* yellow? */
      break;
    case AppState::ChordMode:
      SetRgb1(0.7f,0,1); /* violet/purple */
      // break;
    case AppState::Error:
      SetRgb1(1, 0, 0); /* red */
      pod_.led2.SetRed(1);
      break;
    default:
      break;
  }
  if (curr_state_ != AppState::Synthesis) SetRgb2(0,0,0); /* turn off led2 */
  led1_flash_count_=0;
}

/// @brief sets led 1 colours based on synth mode (regular param controls)
void GrannyChordApp::SetLed1SynthMode(){
  if (curr_state_ == AppState::Synthesis){
    switch(curr_synth_mode_){
      case SynthMode::Size_Position:
        SetRgb1(0, 0.7f, 0); /* green */
        break;
      case SynthMode::Pitch_ActiveGrains:
        SetRgb1(0.9f, 0.6f, 0); /* orange */
        break;
      case SynthMode::Pan_Direction:
        SetRgb1(0,0.7f,0.7f); /* cyan */
        break;
      case SynthMode::Reverb:
        SetRgb1(0.8f, 0, 0.7f); /* pink */
        break;
      default:
        break;
    }
  }
  led2_flash_count_=0;
}
        // SetRgb2(0.9f,0.9f,0); /* yellow */

/// @brief sets led 2 colours based on synth mode (random, regular or FX)
void GrannyChordApp::SetLed2(){
  if (curr_state_==AppState::Synthesis){
    switch(curr_synth_mode_){
      case SynthMode::Size_Position:
      case SynthMode::Pitch_ActiveGrains:
      case SynthMode::Pan_Direction:
      case SynthMode::Reverb:
        SetRgb2(0, 0, 0); /* off in even mode */
        break;
      case SynthMode::Size_Position_Rnd:
        SetRgb2(0, 0.7f, 0); /* green */
        break;
      case SynthMode::Pitch_ActiveGrains_Rnd:
        SetRgb2(0.9f, 0.6f, 0); /* orange */
        break;
      case SynthMode::PanRnd_Chorus:
         SetRgb2(0,0.7f,0.7f); /* cyan */
        break;
      case SynthMode::Filter:
        SetRgb2(0,0.7f,0.7f); /* cyan for reverb */
        break;
      default:
        break;
    }
  }
  led2_flash_count_=0;
}
