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
  // loadmeter.Init(pod_.AudioSampleRate(), pod_.AudioBlockSize());
  InitFX();
  InitPrevParamVals();
  InitColours();
  SetLedAppState();
  pod_.UpdateLeds();
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
  // timer_.Start();
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
    SetLedAppState();
    HandleStateChange();
    DebugPrint(pod_, "handled state change");
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
      DebugPrint(pod_,"starting playback");
      curr_state_ = AppState::PlayWAV;
      break;
    case AppState::Synthesis:
      InitSynth();
      curr_state_ = AppState::Synthesis;
      // SetLedSynthMode();
      // DebugPrint(pod_, "started audiocallback");
      break;
    case AppState::Error: 
      curr_state_ = AppState::Error;
      break;
    default: 
      break;
  }
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
        pod_.seed.SetLed(1);
        recording_out_ = true;
        DebugPrint(pod_,"now recording out!");
        RecordOutToSD();
      } 
      else{
        pod_.seed.SetLed(0);
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
  NextSynthMode();
}

/// @brief Toggles on/off synth parameter randomness controls
void GrannyChordApp::HandleButton2(){
    PrevSynthMode();
}

/// @brief Switches between controlling regular parameter modes and FX parameter modes
void GrannyChordApp::HandleButton1LongPress(){
  int mode_idx = static_cast<int>(curr_synth_mode_);
  if (mode_idx<4) mode_idx = 4;
  else mode_idx = 0;
  curr_synth_mode_ = static_cast<SynthMode>(mode_idx);
  SetLedSynthMode();
}

/// @brief Switches to Chord Mode
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

/// @brief iterates to next synth mode within regular or FX group
void GrannyChordApp::NextSynthMode(){
  DebugPrint(pod_, "going to synth mode");
  int mode_idx = static_cast<int>(curr_synth_mode_);
  /* cycle to next regular mode, indices 0-3 */
  if (mode_idx < 4){
    mode_idx++;
    if (mode_idx>=4) mode_idx =0;
  }
  /* cycle to next FX mode, indices 4-7 */
  else {
    mode_idx++;
    if (mode_idx>7) mode_idx =4;
  }
  curr_synth_mode_ = static_cast<SynthMode>(mode_idx);
  DebugPrintMode(curr_synth_mode_);
  SetLedSynthMode();
  knob1_latched = true;
  knob2_latched = true;
}

/// @brief iterates to previous synth mode within regular or FX group
void GrannyChordApp::PrevSynthMode(){
  DebugPrint(pod_, "going to prev synth mode");
  int mode_idx = static_cast<int>(curr_synth_mode_);
  /* cycle to prev regular mode indices 0-3 */
  if (mode_idx < 4){
    mode_idx--;
    if (mode_idx<0) mode_idx = 3;
  }
  /* cycle to prev FX mode, indices 4-7 */
  else {
    mode_idx--;
    if (mode_idx<4) mode_idx = 7;
  }
  curr_synth_mode_ = static_cast<SynthMode>(mode_idx);
  DebugPrintMode(curr_synth_mode_);
  SetLedSynthMode();
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
  
  // counter++;

  /* only update parameter if knob has passed through previous value in this mode */
  if (UpdateKnobPassThru(&knob1_latched, knob1_val,prev_param_k1[mode_idx])){
    UpdateKnob1Params(knob1_val,curr_synth_mode_);
    prev_param_k1[mode_idx] = knob1_val;
    if (counter%300000==0){
      DebugPrint(pod_, "new k1v: %f",knob1_val);
    }
  }

  if (UpdateKnobPassThru(&knob2_latched, knob2_val,prev_param_k2[mode_idx])){
    UpdateKnob2Params(knob2_val, curr_synth_mode_);
    prev_param_k2[mode_idx] = knob2_val;
    if (counter%300000==0){
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
  /* set regular synth parameters */
  for (int i=0; i < NUM_SYNTH_MODES;i++){
    if (i<3){
      prev_param_k1[i] = 0.5f;
      prev_param_k2[i] = 0.5f;
    }
    /* set randomness and FX values to 0 initially */
    else {
      prev_param_k1[i]=0.0f;
      prev_param_k2[i]=0.0f;
    }
  }
}

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
      return;  
    case AppState::RecordIn:
      // ProcessRecordIn(in, out, size);
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
    if (wav_playhead_ < filemgr_.GetSamplesPerChannel()){
      out[0][i] = s162f(left_buf_[wav_playhead_]) * 0.5f;
      out[1][i] = s162f(right_buf_[wav_playhead_]) * 0.5f;
      wav_playhead_++;
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

    samp = ProcessFX(samp);
  //   out[0][i] = samp.left;
  //   out[1][i] = samp.right;
  // }
    // limiter_.ProcessBlock(&samp.left, 1, 0.0f);
    // limiter_.ProcessBlock(&samp.right, 1, 0.0f);
  //   // if (counter%128000==0){
  //   //   counter=0;
  //   //   DebugPrint(pod_, "l: %f, r: %f", samp.left, samp.right);
  //   // }
    out[0][i] = samp.left;
    out[1][i] = samp.right;

  //   // if (recording_out_ && sd_writer_.GetLengthSeconds()<MAX_REC_OUT_LEN){
  //   //   temp_interleaved_buf_[0]=out[0][i];
  //   //   temp_interleaved_buf_[1]=out[1][i];
  //   //   sd_writer_.Sample(temp_interleaved_buf_);
  //   // }
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
  // out = rotator_.ProcessMix(out);
  /* apply chorus */
  // out = chorus_.ProcessMix(out);

  /* apply reverb */
  reverb_.ProcessMix(out.left, out.right, &out.left, &out.right);

  out.left = hicut_.Process(out.left);
  out.right = hicut_.Process(out.right);
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
      return;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetPitchRatio(knob1_val);
      return;
    case SynthMode::Pan_Direction:
      synth_.SetPan(knob1_val);
      return;
    case SynthMode::RndAmount_RndFreq:
      synth_.SetRndAmount(knob1_val);
      break;
    case SynthMode::StereoRotate:
      rotator_.SetFreq(knob1_val);
      return;
    case SynthMode::Chorus:
      chorus_.SetLfoDepth(knob1_val);
      /* set chorus left/right channel to (synth pan +/- 20% of knob value) */
      chorus_.SetPan(fclamp(synth_.GetPan()+(0.2f*knob1_val),0,1), (fclamp(synth_.GetPan()-(0.2f*knob1_val),0,1)));
      return;     
    case SynthMode::Reverb:
       /* set reverb feedback ie tail length */
      reverb_.SetFeedback(knob1_val);
      return;
    case SynthMode::Filter:
      /* map knob value to frequency range with an exponential curve */
      knob1_val = fmap(knob1_val, LOPASS_LOWER_BOUND, LOPASS_UPPER_BOUND, daisysp::Mapping::EXP);
      /* set cutoff frequency of low pass moog filter */
      lowpass_moog_.SetFreq(knob1_val);
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
      return;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetTargetActiveGrains(knob2_val);
      return;
    case SynthMode::Pan_Direction:
      synth_.SetDirection(knob2_val);
      return;
    case SynthMode::RndAmount_RndFreq:
      synth_.SetRndBias(knob2_val);
      break;
    case SynthMode::StereoRotate:
      rotator_.SetMix(knob2_val);
      return;
    case SynthMode::Chorus:
      chorus_.SetMix(knob2_val);
      return;
    case SynthMode::Reverb:
        reverb_.SetMix(knob2_val);
        return;
    case SynthMode::Filter:
      /* map knob value to frequency range with linear curve */
      knob2_val = fmap(knob2_val, HIPASS_LOWER_BOUND, HIPASS_UPPER_BOUND, daisysp::Mapping::EXP);
      /* set cutoff frequency for high pass filter */
      hipass_.SetFrequency(knob2_val);
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
    case SynthMode::Pitch_ActiveGrains:
      DebugPrint(pod_, "State now in: PitchGrains");
      return;
    case SynthMode::Pan_Direction:
      DebugPrint(pod_, "State now in: PanDirection");
      return;
    case SynthMode::RndAmount_RndFreq:
      DebugPrint(pod_, "State now in: RndAmt/Freq");
      return;
    case SynthMode::StereoRotate:
      DebugPrint(pod_, "State now in: StereoRotate");
      return;
    case SynthMode::Chorus:
      DebugPrint(pod_, "State now in: Chorus");
      return;
    case SynthMode::Reverb:
      DebugPrint(pod_, "State now in: Reverb");
      return;
    case SynthMode::Filter:
      DebugPrint(pod_, "State now in: Filter");
      return;
  }
};

void GrannyChordApp::InitColours(){
  colours.BLUE.Init(Color::PresetColor::BLUE);
  colours.GREEN.Init(Color::PresetColor::GREEN);
  colours.RED.Init(Color::PresetColor::RED);
  colours.CYAN.Init(0,255,255);
  colours.PURPLE.Init(Color::PresetColor::PURPLE);
  colours.ORANGE.Init(Color::PresetColor::GOLD);
  colours.YELLOW.Init(255,255,0);
  colours.PINK.Init(255,0,255);
}

/// @brief sets led colours based on app state
void GrannyChordApp::SetLedAppState(){
  switch(next_state_){
    case AppState::SelectFile:
      pod_.led1.SetColor(colours.GREEN);
      break;
    case AppState::PlayWAV:
      pod_.led1.SetColor(colours.CYAN);
      break;
    case AppState::Synthesis:
      pod_.led1.SetColor(colours.BLUE);
      pod_.led2.SetColor(colours.GREEN);
      break;
    case AppState::RecordIn:
      pod_.led1.SetColor(colours.YELLOW);
      break;
    case AppState::ChordMode:
      pod_.led1.SetColor(colours.PURPLE);
      break;
    case AppState::Error:
      pod_.led1.SetColor(colours.RED);
      pod_.led2.SetColor(colours.RED);
      break;
    default:
      break;
  }
  if (curr_state_ != AppState::Synthesis) pod_.led2.Set(0,0,0); /* turn off led2 */
  pod_.UpdateLeds();
}

void GrannyChordApp::SetLedSynthMode(){
  if (curr_state_ == AppState::Synthesis){
    switch(curr_synth_mode_){
      /* regular: led1 blue / cyan / orange / yellow */
      /* FX: led2 blue / cyan / orange / yellow */
      case SynthMode::Size_Position:
        pod_.led1.SetColor(colours.BLUE);
        break;
      case SynthMode::Pitch_ActiveGrains:
        pod_.led1.SetColor(colours.CYAN);
        break;
      case SynthMode::Pan_Direction:
        pod_.led1.SetColor(colours.ORANGE);
        break;
      case SynthMode::RndAmount_RndFreq:
        pod_.led1.SetColor(colours.YELLOW);
        break;
      case SynthMode::StereoRotate:
        pod_.led2.SetColor(colours.BLUE);
        break;
      case SynthMode::Chorus:
        pod_.led2.SetColor(colours.CYAN);
        break;
      case SynthMode::Reverb:
      pod_.led2.SetColor(colours.ORANGE);
        break;
      case SynthMode::Filter:
        pod_.led2.SetColor(colours.YELLOW);
        break;
      default:
        break;
    }
    int mode_idx = static_cast<int>(curr_synth_mode_);
    if (mode_idx < 4) {
      /* led2 green in regular mode */
      pod_.led2.SetColor(colours.GREEN);
    }
    else{
      /* led1 pink in FX Mode */
      pod_.led1.SetColor(colours.PINK);
    }
  }
  pod_.UpdateLeds();
}