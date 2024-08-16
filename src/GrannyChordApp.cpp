#include "GrannyChordApp.h"

/* this is required for AudioCallback as it needs a static object */
GrannyChordApp* GrannyChordApp::instance_ = nullptr;

/// @brief Initialises app state and members and goes through app startup process
/// @param left Left channel audio data buffer
/// @param right Right channel audio data buffer
void GrannyChordApp::Init(int16_t *left, int16_t *right){
  left_buf_ = left;
  right_buf_ = right;
  pod_.SetAudioBlockSize(4);
  pod_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  DebugPrint(pod_,"set sample rate");
  ui_.Init();
  DebugPrint(pod_,"init ui");

  if (!InitFileMgr()){
    ui_.SetStateError();
    DebugPrint(pod_,"filemgr init error");
    return;
  }

  InitCompressor();
  InitPrevParamVals();
  SeedRng();
  
  pod_.StartAdc();
  pod_.StartAudio(AudioCallback);
  DebugPrint(pod_,"started audio");
  // prev_state_ = AppState::Startup;
  // curr_state_ = AppState::SelectFile;
  changing_state_ = true;
}

/// @brief Loops whilst app is running, updating state and UI, handling 
///        state transitions, updating parameters and managing audio recordings
void GrannyChordApp::Run(){
  while(true){
    prev_state_ = ui_.GetCurrentState();
    ui_.UpdateUI();
    curr_state_ = ui_.GetCurrentState();

    if (prev_state_ != curr_state_){
      DebugPrint(pod_, "handling state change");
      HandleStateChange();
      ui_.DebugPrintState();
    }
    // /* check for hardware input-triggered state change */
    // ui_.UpdateUI();
    // AppState ui_state = ui_.GetCurrentState();
    // if (ui_state != curr_state_){
    //   RequestStateChange(ui_state);
    //   DebugPrint(pod_,"requested state change");
    // }

    /* handle pending state changes */
    // if (changing_state_){
    //   DebugPrint(pod_, "handling state change");
    //   HandleStateChange();
    //   changing_state_ = false;
    //   ui_.DebugPrintState();
    // }

    UpdateSynthParams();
    if (ui_.ToggleRecordOut()){
      if (recording_out_) StopRecordOut();
      else StartRecordOut();
    }
    System::Delay(1);
  }
}

/// @brief Initialises file manager, sets audio data buffers and scans SD card for WAV files
/// @return True on successful initialisation, false if init fails or no WAV files found
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

/// @brief Calls synth initialisation function, passes audio data buffers and audio length
void GrannyChordApp::InitSynth(){
  synth_.Init(left_buf_, right_buf_, audio_len_);
}

/// @brief Resets current file index and audio data buffers
///        Calls method to handle WAV file selection
void GrannyChordApp::InitFileSelection(){
  // file_idx_ =0;
  memset(left_buf_, 0, CHNL_BUF_SIZE_ABS);
  memset(right_buf_, 0, CHNL_BUF_SIZE_ABS);
  HandleFileSelection();
}

/// @brief Initialises WAV playback state, resets playhead, sets current file audio length
void GrannyChordApp::InitPlayback(){
  wav_playhead_ = 0;
  audio_len_ = filemgr_.GetSamplesPerChannel();
  #ifdef DEBUG_MODE
  float  total_secs = static_cast<float>(audio_len_)/48000.0f;
  int mins = static_cast<int>(total_secs/60.0f);
  int secs = static_cast<int>(total_secs)%60;
  DebugPrint(pod_,"File loaded: %d:%d ", mins,secs);
  #endif
}

/// @brief Initialises RecordIn state, clears audio buffers
void GrannyChordApp::InitRecordIn(){
  memset(left_buf_, 0, CHNL_BUF_SIZE_ABS);
  memset(right_buf_, 0, CHNL_BUF_SIZE_ABS);
  DebugPrint(pod_, "cleared audio buffers for recording in");
}

/// @brief Initialise object for recording out to SD card
void GrannyChordApp::InitWavWriter(){
  WavWriter<16384>::Config cfg;
  cfg.bitspersample = BIT_DEPTH;
  cfg.channels = filemgr_.GetNumChannels();
  cfg.samplerate = SAMPLE_RATE;
  sd_out_writer_.Init(cfg);
}

/// @brief Initialise compressor object with desired parameters
void GrannyChordApp::InitCompressor(){
  comp_.Init(pod_.AudioSampleRate());
  comp_.SetRatio(3.0f);
  comp_.SetAttack(0.01f);
  comp_.SetRelease(0.1f);
  comp_.SetThreshold(-12.0f);
  comp_.AutoMakeup(true);
}

void GrannyChordApp::InitReverb(){
  reverb_.Init(SAMPLE_RATE_FLOAT);
}

void GrannyChordApp::InitFilters(){
  lowpass_moog_.Init(SAMPLE_RATE_FLOAT);
  lowpass_moog_.SetFreq(20000.0f);
  lowpass_moog_.SetRes(0.7f);
  hipass_onepole_.Init();
  hipass_onepole_.SetFilterMode(daisysp::OnePole::FilterMode::FILTER_MODE_HIGH_PASS);
  hipass_onepole_.SetFrequency(0.0f); // NOTE: CHECK THIS
}
/// @brief Initialise previous parameter value arrays to defaults
void GrannyChordApp::InitPrevParamVals(){
      for (int i=0; i < NUM_SYNTH_MODES;i++){
        prev_param_vals_k1[i] = 0.5f;
        prev_param_vals_k2[i] = 0.5f;
      } 
};

/// @brief Master audio callback, called at audio rate, calls all audio processing methods
/// @param in Audio input buffer
/// @param out Audio output buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  AppState curr = instance_->ui_.GetCurrentState();
  // if in RecordIn: clear channel buffers, send input straight to them
  // track audio length, if greater than (60*48000) ie 60s, wrap index
  // to front of buffer so it's circular and can't record > 60s
  // then set audio_len_ vbl to tracked length so synth works properly
  switch (curr){
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

/// @brief Outputs silence if app is in non audio output state
/// @param out Output audio buffer
/// @param size Number of samples to process in this call (irrelevant)
void GrannyChordApp::AudioIdle(AudioHandle::OutputBuffer out, size_t size){
  for (size_t i=0; i<size; i++){
    out[0][i]=out[1][i]=0.0f;
  }
}

/// @brief Process audio for WAV playback mode and increment playback position
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
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

/// @brief Process input audio when app is in RecordIn state
/// @param in Input audio buffer
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
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

/// @brief Process audio through granular synth and mix to output buffer
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size){
  // for (size_t i=0; i<size; i++){ // NOTE: is this correct? do i need to loop? don't think so
    synth_.ProcessGrains(out[0],out[1],size); 
    comp_.ProcessBlock(out[0],out[0],size);
    comp_.ProcessBlock(out[1],out[1],size);

    float temp_left[size];
    float temp_right[size];
    
    for (size_t i=0; i<size; i++){
      temp_left[i] = out[0][i];
      temp_right[i] = out[1][i];
      reverb_.Process(temp_left[i],temp_right[i],&out[0][i],&out[1][i]); // NOTE: check this works
    }
}

/// @brief Process audio from chord mode and mix to output buffer
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::ProcessChordMode(AudioHandle::OutputBuffer out, size_t size){
  // if (UserTriggeredChord()) {
  //   synth_.TriggerChord(out[0], out[1], size);
  // } else {
  //     std::memset(out[0], 0, size * sizeof(float));
  //     std::memset(out[1], 0, size * sizeof(float));
  // }
}

/// @brief Record granular synth or chord output audio to SD card
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::RecordOutToSD(AudioHandle::OutputBuffer out, size_t size){
  InitWavWriter();
  snprintf(recording_fname_, sizeof(recording_fname_), "recording %d", recording_count_);
  sd_out_writer_.OpenFile(recording_fname_);
  // NOTE: what now? do i just record in audiocallback? 

//    Open a new file for writing with: writer.OpenFile("FileName.wav")
//  ** 5. Write to it within your audio callback using: writer.Sample(value)
//  ** 6. Fill the Wav File on the SD Card with data from your main loop by running: writer.Write()
//  ** 7. When finished with the recording finalize, and close the file with: writer.SaveFile();
// NOTE: 
// USE THIS 
// https://electro-smith.github.io/libDaisy/classdaisy_1_1_wav_writer.html

  // create file on sd card 
  // write in blocks]
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

  // don't stop audio since i'm recording
}

/// @brief Helper function to mark when audio output recording starts
void GrannyChordApp::StartRecordOut(){
  recording_out_=true;
}

/// @brief Helper function to mark when audio output recording starts
void GrannyChordApp::StopRecordOut(){
  recording_out_ = false;
}

/// @brief Handles transitions between states, preparing to change to next state
/// @param prev Previous state of the app
/// @param curr State that the app is about to change to
void GrannyChordApp::HandleStateChange(){
  /* clean up from previous state */
  switch (prev_state_){
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
    default:
      break;
  }
  /* prepare for new state */
  switch(curr_state_){
    case AppState::SelectFile:
      DebugPrint(pod_, "init file selection now");
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
    case AppState::Error:
      // Handle error state? what to do....
      break;
    default:
      // what here? restart ? 
      break;
  }
}


/// @brief Handles file selection. Processes hardware input controls on loop
///        until a file has been selected and loaded, then starts playback
/// @return True if file loads ok or state changes to RecordIn
///         False if called in wrong state, or file doesn't load
void GrannyChordApp::HandleFileSelection(){
  if (curr_state_ != AppState::SelectFile){
    DebugPrint(pod_,"not in select file state");
  }
  while (true){
    pod_.ProcessDigitalControls();
    if (ui_.EncoderLongPress()){
      DebugPrint(pod_,"requesting change to RecordIn");
      RequestStateChange(AppState::RecordIn);
      return;
    }
    int32_t inc = ui_.GetEncoderIncrement();
    uint16_t file_count = filemgr_.GetFileCount();
    if (inc!=0){
      file_idx_ += inc;
      /* wrap around list of files */
      if (file_idx_<0){ 
        file_idx_ = file_count - 1; 
      }
      if (file_idx_ >= file_count){ 
        file_idx_ = 0; 
      }
      filemgr_.GetName(file_idx_, fname);
      DebugPrint(pod_,"selected new file idx %d %s", file_idx_, fname);
    }
    if (ui_.EncoderPressed()){
      DebugPrint(pod_,"about to load file");
      if (filemgr_.LoadFile(file_idx_)){
        DebugPrint(pod_, "loading file %s",fname);
        RequestStateChange(AppState::PlayWAV);
        return;
      } 
      else {
        DebugPrint(pod_,"Failed to load audio file");
        RequestStateChange(AppState::Error);
        return;
      }
    }
  }
}

/// @brief Updates granular synth or chord parameters if in Synthesis/ChordMode state
void GrannyChordApp::UpdateSynthParams(){
  if (curr_state_==AppState::Synthesis){
    UpdateGranularParams();
  }
  // else if (curr_state==AppState::ChordMode){
  //   UpdateChordParams();
  // }
}

/// @brief Updates granular synth parameters based on hardware input and current synth mode
void GrannyChordApp::UpdateGranularParams(){
  SynthMode mode = ui_.GetCurrentSynthMode();
  int mode_idx = static_cast<int>(mode);
  float knob1_val = ui_.GetKnob1Value(mode_idx);
  float knob2_val = ui_.GetKnob2Value(mode_idx);
  /* Check knob has moved a significant amount to avoid changes due to knob jitter */
  if (CheckParamDelta(knob1_val, prev_param_vals_k1[mode_idx])){
    /* Set parameter for knob 1 depending on current synth mode */
    UpdateKnob1Params(knob1_val, mode);
    /* Update previous parameter value for pass-thru mode */
    prev_param_vals_k1[mode_idx] = knob1_val;
  }
  if (CheckParamDelta(knob2_val, prev_param_vals_k2[mode_idx])){
    /* Set parameter for knob 2 depending on current synth mode */
    UpdateKnob2Params(knob2_val, mode);
    /* Update previous parameter value for pass-thru mode */
    prev_param_vals_k2[mode_idx] = knob2_val;
  }
}

void GrannyChordApp::UpdateKnob1Params(float knob1_val, SynthMode mode){
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
    case SynthMode::Reverb:
      /* set reverb feedback ie tail length */
      reverb_.SetFeedback(knob1_val); // NOTE check it doesn't clip
      break;
    case SynthMode::Filter:
      // knob1_val = fmap(knob1_val, 20.0f, 20000.0f, daisysp::Mapping::EXP);
      /* map knob value to frequency range with an exponential curve */
      knob1_val = fmap(knob1_val, 20.0f, 5000.0f, daisysp::Mapping::EXP);
      /* set cutoff frequency of low pass moog filter */
      lowpass_moog_.SetFreq(knob1_val);
      break;
  }
}

void GrannyChordApp::UpdateKnob2Params(float knob2_val, SynthMode mode){
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
    case SynthMode::Reverb:
      /* map knob value to frequency range with an exponential curve */
      knob2_val = fmap(knob2_val, 100.0f, 20000.0f, daisysp::Mapping::EXP);
      /* set reverb low pass frequency */
      reverb_.SetLpFreq(knob2_val);
      break;
    case SynthMode::Filter:
      /* again, map knob value to frequency range with an exponential curve */
      knob2_val = fmap(knob2_val, 0.001f, 0.497f, daisysp::Mapping::EXP);
      /* set cutoff frequency for high pass filter */
      hipass_onepole_.SetFrequency(knob2_val);
      break;
  }
}

/// @brief Check parameter value change is over a certain level, due to knob jitter
/// @param curr_val Current value of hardware input knob
/// @param prev_val Previous value of hardware input knob
/// @return True if parameter change is significant, else false
bool GrannyChordApp::CheckParamDelta(float curr_val, float prev_val){
  return (fabsf(curr_val - prev_val)>0.01f);
}



// void GrannyChordApp::UpdateFXParams(){

// }


