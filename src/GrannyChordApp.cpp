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
  InitColours();
  SetLedAppState();
  pod_.UpdateLeds();
  SeedRng();
  synth_.Init(left_buf_, right_buf_, 0);
  pod_.StartAdc();
}

/// @brief Loops whilst app is running, updating state and UI, handling 
///        state transitions, updating parameters and managing audio recordings
void GrannyChordApp::Run(){
  pod_.seed.PrintLine("app is running");
  while(true){
    if (recording_out_) {
      sd_writer_.Write();
      if (sd_writer_.GetLengthSeconds()>120){
        sd_writer_.SaveFile();
        recording_out_ = false;
      }
    }
    UpdateUI();
    UpdateParams();
    System::Delay(1);
  }
}

/// @brief Handles hardware control inputs and calls state change methods
void GrannyChordApp::UpdateUI(){
  pod_.ProcessDigitalControls();

  if (pod_.encoder.TimeHeldMs() > 1000.0f) HandleEncoderLongPress();
  else if (pod_.encoder.FallingEdge()) HandleEncoderPressed();

  if (curr_state_==AppState::Synthesis || curr_state_==AppState::ChordMode){
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
    DebugPrint(pod_,"switched state\n");
    #endif
    SetLedAppState();
    HandleStateChange();
  }
}

/// @brief handles transitions between states and prepares for next state
void GrannyChordApp::HandleStateChange(){
  switch(next_state_){
    case AppState::SelectFile:
      pod_.StopAudio();
      curr_state_ = AppState::SelectFile;
      break; 
    case AppState::RecordIn:
      InitRecordIn();
      curr_state_ = AppState::RecordIn;  
      break;
    case AppState::PlayWAV:
      InitPlayback(); 
      curr_state_ = AppState::PlayWAV;
      break;
    case AppState::Synthesis:
      if (curr_state_ != AppState::ChordMode){
        InitSynth();
      }
      curr_state_ = AppState::Synthesis;
      break;
    case AppState::ChordMode:
      curr_state_ = AppState::ChordMode;
      break;
    case AppState::Error: 
      curr_state_ = AppState::Error;
      break;
  }
}

void GrannyChordApp::UpdateParams(){
  if (curr_state_==AppState::Synthesis){
    UpdateSynthParams();
  }
  else if (curr_state_==AppState::ChordMode){
    ChangeChordKey();
    ChangeChordSpawnPos();
  }
  else return;
}

/// @brief Handles when encoder is scrolled to select a file 
/// @param encoder_inc amount the encoder has been scrolled/incremented
void GrannyChordApp::HandleEncoderIncrement(int encoder_inc){
  if (curr_state_==AppState::PlayWAV){
    recorded_in_ = false;
    next_state_ = AppState::SelectFile;
  }
  if (next_state_== AppState::SelectFile){
    recorded_in_ = false;
    HandleFileSelection(encoder_inc);
  }

  if (curr_state_ == AppState::ChordMode){
    std::vector<float> ratios = chord_gen_.GetRatios(encoder_inc);
    synth_.EnqueueChord(ratios);
    DebugPrint(pod_, "queued chord ");
    System::Delay(5);
    for (size_t i =0; i<ratios.size(); i++){
      pod_.seed.Print("%f ", ratios[i]);
    }
    pod_.seed.Print("\n");
    DebugPrint(pod_, "curr step %d", chord_gen_.GetStep());
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
    case AppState::Synthesis:
      next_state_ = AppState::ChordMode;
      return;
    case AppState::ChordMode:
      next_state_= AppState::Synthesis;
      return;
    default:
      return;
  }
}

/// @brief switch between app states by long pressing encoder.
void GrannyChordApp::HandleEncoderLongPress(){
  do { pod_.encoder.Debounce();  }
  while(!pod_.encoder.FallingEdge());
  switch(curr_state_){
    case AppState::Synthesis:
    case AppState::RecordIn:
    case AppState::ChordMode: 
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

void GrannyChordApp::ButtonHandler(){
  if (pod_.button1.TimeHeldMs()>500.0f){
    while (!pod_.button1.FallingEdge()){
      pod_.button1.Debounce();
    }
    HandleButton1LongPress();
  }

  else if (pod_.button1.FallingEdge()){
    HandleButton1();
  }

  if (pod_.button2.FallingEdge()){
    HandleButton2();
  }

}

/// @brief Switch between synth parameter control modes
void GrannyChordApp::HandleButton1(){
  switch (curr_state_){
    case AppState::Synthesis:
      NextSynthMode();
      return;
    case AppState::ChordMode:
      CycleChordPlaybackMode();
      return;
    default:
      return;
  }
}

/// @brief Toggles on/off synth parameter randomness controls
void GrannyChordApp::HandleButton2(){
  if (curr_state_==AppState::Synthesis){
    PrevSynthMode();
  }
  else if (curr_state_==AppState::ChordMode){
    CycleChordScale();
  }
}

// /// @brief Toggles recording out to SD card
void GrannyChordApp::HandleButton1LongPress(){
  if (curr_state_==AppState::Synthesis || curr_state_==AppState::ChordMode){
    if (!recording_out_){
      /* set seed led whilst recording out */
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
  }
  else return;
}

/// @brief Scrolls through list of files for user selection
void GrannyChordApp::HandleFileSelection(int32_t encoder_inc){
  file_led_state_ = !file_led_state_;
  if (file_led_state_){
    pod_.led1.SetColor(colours.GREEN);
  }
  else {
    pod_.led1.SetColor(colours.OFF);
  }
  pod_.UpdateLeds();
  int file_count = filemgr_.GetFileCount();
  file_idx_ = (file_idx_ + encoder_inc + file_count) % file_count;
  filemgr_.GetName(file_idx_,fname_);
  DebugPrint(pod_, "selected file %d %s",file_idx_,fname_);
  System::Delay(5);
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
  limiter_.Init();
  reverb_.Init(SAMPLE_RATE_FLOAT);
  reverb_.SetMix(0.0f);
  reverb_.SetFeedback(0.0f);
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
      prev_k1_pos[i]=0.5f;
      prev_k2_pos[i]=0.5f;
    }
    /* set reverb values to 0.05 initially */
    else {
      prev_param_k1[i]=0.05f;
      prev_param_k2[i]=0.05f;
      prev_k1_pos[i]=0.05f;
      prev_k2_pos[i]=0.05f;
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
      ProcessRecordIn(in, out, size);
      return;
    case AppState::Synthesis:
      ProcessSynthesis(out,size, false);
      return;
    case AppState::ChordMode:
      ProcessSynthesis(out,size, true);
    return;
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
      out[0][i] = s162f(left_buf_[wav_playhead_]);
      out[1][i] = s162f(right_buf_[wav_playhead_]);
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
void GrannyChordApp::ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size, bool process_chord){
  Sample samp;
  for (size_t i=0; i<size; i++){ 
    if (process_chord){
      if (!synth_.ChordActive() && !synth_.ChordQueueEmpty()){
        synth_.TriggerChord();
      }
      samp = synth_.ProcessChord();
    }
    else {
      samp = synth_.ProcessGrains();
    }

    Sample processed = ProcessFX(samp);
    limiter_.ProcessBlock(&processed.left, 1, 0.5f);
    limiter_.ProcessBlock(&processed.right, 1, 0.5f);
    out[0][i] = processed.left;
    out[1][i] = processed.right;

    if (recording_out_ && sd_writer_.GetLengthSeconds()<MAX_REC_OUT_LEN){
      temp_interleaved_buf_[0]=out[0][i];
      temp_interleaved_buf_[1]=out[1][i];
      sd_writer_.Sample(temp_interleaved_buf_);
    }
  }
}

Sample GrannyChordApp::ProcessFX(Sample in){
  Sample out;
  /* hipass: remove rumble */
  out.left = hipass_.Process(in.left);
  out.right = hipass_.Process(in.right);
  // out.left = comp_.Process(in.left);
  // out.right = comp_.Process(in.right);

  /* apply lowpass filter */
  out.left = lowpass_moog_.Process(out.left);
  out.right = lowpass_moog_.Process(out.right);

  /* apply reverb */
  reverb_.ProcessMix(out.left, out.right, &out.left, &out.right);
  out.left = hicut_.Process(out.left);
  out.right = hicut_.Process(out.right);

  limiter_.ProcessBlock(&out.left, 1, 0.5f);
  limiter_.ProcessBlock(&out.right, 1, 0.5f);

  return out;
}

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

/// @brief iterates to next synth mode within regular or FX group
void GrannyChordApp::NextSynthMode(){
  DebugPrint(pod_, "going to synth mode");
  int mode_idx = static_cast<int>(curr_synth_mode_);
  prev_k1_pos[mode_idx] = MapKnobDeadzone(pod_.knob1.Process());
  prev_k2_pos[mode_idx] = MapKnobDeadzone(pod_.knob2.Process());
  mode_idx++;
  if (mode_idx>3) mode_idx =0;
  curr_synth_mode_ = static_cast<SynthMode>(mode_idx);
  DebugPrintMode(curr_synth_mode_);
  SetLedSynthMode();
  prev_k1_pos[mode_idx] = MapKnobDeadzone(pod_.knob1.Process());
  prev_k2_pos[mode_idx] = MapKnobDeadzone(pod_.knob2.Process());
  knob1_latched = false;
  knob2_latched = false;
}

/// @brief iterates to previous synth mode within regular or FX group
void GrannyChordApp::PrevSynthMode(){
  DebugPrint(pod_, "going to prev synth mode");
  int mode_idx = static_cast<int>(curr_synth_mode_);
  prev_k1_pos[mode_idx] = MapKnobDeadzone(pod_.knob1.Process());
  prev_k2_pos[mode_idx] = MapKnobDeadzone(pod_.knob2.Process());
  // knob1_latched = false;
  // knob2_latched = false;  
  mode_idx --;
  if (mode_idx<0) mode_idx=3;
  curr_synth_mode_ = static_cast<SynthMode>(mode_idx);
  prev_k1_pos[mode_idx] = MapKnobDeadzone(pod_.knob1.Process());
  prev_k2_pos[mode_idx] = MapKnobDeadzone(pod_.knob2.Process());
  knob1_latched = false;
  knob2_latched = false;  
  DebugPrintMode(curr_synth_mode_);
  SetLedSynthMode();
}

/// @brief Updates synth parameters based on current synth mode and adjusts
///        knob input values to account for knob jitter and deadzones around 0/1
void GrannyChordApp::UpdateSynthParams(){
  int mode_idx = static_cast<int>(curr_synth_mode_);
  /* set to 0 or 1 if very close to these bounds */
  float knob1_val = MapKnobDeadzone(pod_.knob1.Process());
  float knob2_val = MapKnobDeadzone(pod_.knob2.Process());


  if (!knob1_latched) {
    if ((knob1_val >= prev_param_k1[mode_idx] && prev_k1_pos[mode_idx] <= prev_param_k1[mode_idx]) ||
        (knob1_val <= prev_param_k1[mode_idx] && prev_k1_pos[mode_idx] >= prev_param_k1[mode_idx])) {
      knob1_latched = true;
    }
  }
  if (knob1_latched){
    UpdateKnob1SynthParams(knob1_val, curr_synth_mode_);
    prev_param_k1[mode_idx] = knob1_val;
    prev_k1_pos[mode_idx] = knob1_val;
  }


  if (!knob2_latched) {
    if ((knob2_val >= prev_param_k2[mode_idx] && prev_k2_pos[mode_idx] <= prev_param_k2[mode_idx]) ||
        (knob2_val <= prev_param_k2[mode_idx] && prev_k2_pos[mode_idx] >= prev_param_k2[mode_idx])) {
      knob2_latched = true;
      DebugPrint(pod_,"latched 2");
    }
  }
 

  if (knob2_latched){
    UpdateKnob2SynthParams(knob2_val, curr_synth_mode_);
    prev_param_k2[mode_idx] = knob2_val;
    prev_k2_pos[mode_idx] = knob2_val;
  }
  System::Delay(5);
}

/// @brief Updates synth parameters from hardware knob 1 input
/// @param knob1_val float between 0-1 from knob input
/// @param mode current synth mode that determines which parameters to update
void GrannyChordApp::UpdateKnob1SynthParams(float knob1_val, SynthMode mode){
  switch (mode){
    case SynthMode::Size_Position:
      synth_.SetGrainSize(knob1_val);
      break;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetPitchRatio(knob1_val);
      break;
    case SynthMode::Reverb:
       /* set reverb feedback ie tail length */
      reverb_.SetFeedback(knob1_val);
      break;
    case SynthMode::Filter:
      /* map knob value to frequency range with an exponential curve */
      knob1_val = fmap(knob1_val, LOPASS_LOWER_BOUND, LOPASS_UPPER_BOUND, daisysp::Mapping::EXP);
      /* set cutoff frequency of low pass moog filter */
      lowpass_moog_.SetFreq(knob1_val);
      break;
  }
}

/// @brief Updates synth parameters from hardware knob 2 input
/// @param knob1_val float between 0-1 from knob input
/// @param mode current synth mode that determines which parameters to update
void GrannyChordApp::UpdateKnob2SynthParams(float knob2_val, SynthMode mode){
  switch (mode){
    case SynthMode::Size_Position:
      synth_.SetSpawnPos(knob2_val);
      break;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetTargetActiveGrains(knob2_val);
      break;
    case SynthMode::Reverb:
      reverb_.SetMix(knob2_val);
      break;
    case SynthMode::Filter:
      /* map knob value to frequency range with linear curve */
      knob2_val = fmap(knob2_val, HIPASS_LOWER_BOUND, HIPASS_UPPER_BOUND, daisysp::Mapping::EXP);
      /* set cutoff frequency for high pass filter */
      hipass_.SetFrequency(knob2_val);
      break;
  }
}

void GrannyChordApp::CycleChordPlaybackMode(){
  chord_gen_.CyclePlaybackMode();
  std::string s = chord_gen_.GetModeName();
  DebugPrint(pod_, "%s", s);
}

void GrannyChordApp::CycleChordScale(){
  std::string s;
  switch (chord_gen_.GetMode()){
    case ChordPlaybackMode::Chord:
      chord_gen_.CycleChord();
      s = chord_gen_.GetChordName();
      DebugPrint(pod_, "%s", s);
      return;
    case ChordPlaybackMode::Arpeggio:
    case ChordPlaybackMode::Scale:
      chord_gen_.CycleScale();
      s = chord_gen_.GetScaleName();
      DebugPrint(pod_, "%s", s);
      return;
  }
}

void GrannyChordApp::ChangeChordKey(){
  float knob_val = MapKnobDeadzone(pod_.knob1.Process());
  knob_val = round(fmap(knob_val, 0.0f, 12.0f));
  int key = static_cast<int>(knob_val);
  chord_gen_.SetKey(key);
}

void GrannyChordApp::ChangeChordSpawnPos(){
  float knob_val = MapKnobDeadzone(pod_.knob2.Process());
  synth_.SetSpawnPos(knob_val);
  /* set synth parameter tracking arrays with new value */
  prev_param_k2[0] = knob_val;
  prev_k2_pos[0] = knob_val;
}

/* knobs on the Pod have deadzones around 0 and 1 so we adjust for this */
inline float GrannyChordApp::MapKnobDeadzone(float knob_val ){
  if (knob_val<=0.01f) return 0.0f;
  if (knob_val>=0.99f) return 1.0f;
  return knob_val;
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
    case AppState::ChordMode:
      DebugPrint(pod_, "State now in: ChordMode");
      return;
    case AppState::Error:
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
  colours.ORANGE.Init(Color::PresetColor::GOLD);
  colours.YELLOW.Init(255,255,0);
  colours.PINK.Init(255,0,255);
  colours.OFF.Init(0,0,0);
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
      break;
    case AppState::RecordIn:
      pod_.led1.SetColor(colours.YELLOW);
      break;
    case AppState::ChordMode:
      pod_.led1.SetColor(colours.PINK);
      break;
    case AppState::Error:
      pod_.led1.SetColor(colours.RED);
      pod_.led2.SetColor(colours.RED);
      break;
    default:
      break;
  }
  if (next_state_ != AppState::Synthesis) pod_.led2.SetColor(colours.OFF); /* turn off led2 */
  pod_.UpdateLeds();
}

void GrannyChordApp::SetLedSynthMode(){
  if (curr_state_ == AppState::Synthesis){
    switch(curr_synth_mode_){
      /* led2 blue / cyan / orange / yellow */
      case SynthMode::Size_Position:
        pod_.led2.SetColor(colours.BLUE);
        break;
      case SynthMode::Pitch_ActiveGrains:
        pod_.led2.SetColor(colours.CYAN);
        break;
      case SynthMode::Reverb:
      pod_.led2.SetColor(colours.YELLOW);
        break;
      case SynthMode::Filter:
        pod_.led2.SetColor(colours.GREEN);
        break;
      default:
        break;
    }
  }
  else pod_.led2.SetColor(colours.OFF);
  pod_.UpdateLeds();
}

void GrannyChordApp::SetLedChordMode(){
  if (curr_state_==AppState::ChordMode){
    ChordPlaybackMode mode = chord_gen_.GetMode();
    switch(mode){
      case ChordPlaybackMode::Chord:
        pod_.led2.SetColor(colours.BLUE);
        break;
      case ChordPlaybackMode::Scale:
        pod_.led2.SetColor(colours.CYAN);
        break;
      case ChordPlaybackMode::Arpeggio:
        pod_.led2.SetColor(colours.YELLOW);
        break;
    }
  }
  else pod_.led2.SetColor(colours.OFF);
  pod_.UpdateLeds();
}