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
void GrannyChordApp::Init(int16_t *left, int16_t *right, int16_t *temp){
  left_buf_ = left, right_buf_ = right, temp_buf_ = temp;
  pod_.SetAudioBlockSize(4);
  pod_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  curr_state_ = AppState::SelectFile;
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
  SeedRng();

  pod_.StartAdc();
}

/// @brief Loops whilst app is running, updating state and UI, handling 
///        state transitions, updating parameters and managing audio recordings
void GrannyChordApp::Run(){
  while(true){
    UpdateUI();
    System::Delay(10);
  }
}

void GrannyChordApp::UpdateUI(){
  AppState prev_state = curr_state_;
  pod_.ProcessDigitalControls();
  if (pod_.encoder.Pressed()) HandleEncoderPressed();
  if (pod_.encoder.TimeHeldMs() > 1000.0f) HandleEncoderLongPress();
  if (pod_.button1.Pressed()) HandleButton1();
  if (pod_.button2.Pressed()) HandleButton2();
  if (curr_state_==AppState::Synthesis){
    if (pod_.button1.TimeHeldMs() >1000.0f) HandleButton1LongPress();
    if (pod_.button2.TimeHeldMs() >1000.0f) HandleButton2LongPress();
  }

  int32_t encoder_inc = pod_.encoder.Increment();
  if (encoder_inc!=0){
    if (curr_state_== AppState::SelectFile){
      HandleFileSelection(encoder_inc);
    }
    else if (curr_state_==AppState::PlayWAV){
      curr_state_ = AppState::SelectFile;
    }
  }
  if (prev_state != curr_state_){
    DebugPrintState(prev_state);
    DebugPrint(pod_, "switching states");
    DebugPrintState(curr_state_);
    HandleStateChange();
  }
}

void GrannyChordApp::HandleStateChange(){
  switch(curr_state_){
    case AppState::SelectFile:
      pod_.StopAudio();
      return; 
    case AppState::RecordIn:
      ClearAudioBuffers();    
      return;
    case AppState::PlayWAV:
      InitPlayback(); 
      return;
    case AppState::Synthesis:
      InitSynth();
      return;
    case AppState::Error: 
      pod_.led1.SetRed(1); 
      pod_.led2.SetRed(1); 
      return;
    default: 
      return;
  }
}

void GrannyChordApp::HandleEncoderPressed(){
  switch(curr_state_){
    case AppState::SelectFile:
      curr_state_ = AppState::PlayWAV;
      return;
    case AppState::RecordIn: 
      curr_state_ = AppState::PlayWAV;
      return;
    case AppState::PlayWAV:
      curr_state_ = AppState::Synthesis;
      return;
    default:
      return;
  }
}

void GrannyChordApp::HandleEncoderLongPress(){
  switch(curr_state_){
    case AppState::Synthesis:
      curr_state_ = AppState::SelectFile;
      return;
    case AppState::RecordIn:  
      curr_state_ = AppState::SelectFile;
      return;
    case AppState::SelectFile:
      curr_state_ = AppState::RecordIn; 
      return;
    case AppState::PlayWAV: 
      curr_state_ = AppState::RecordIn;  
      return;
    default:
      return;
  }
}

void GrannyChordApp::HandleButton1(){
  if (curr_state_==AppState::Synthesis){
    UpdateSynthMode();
  }
}

/// @brief Toggles on/off randomness for synth parameters
void GrannyChordApp::HandleButton2(){
  if (curr_state_==AppState::Synthesis){
    ToggleRandomnessControls();
  }
}

void GrannyChordApp::HandleButton1LongPress(){
  if (curr_state_==AppState::Synthesis){
    ToggleFX(true);
  }
}

void GrannyChordApp::HandleButton2LongPress(){
  if (curr_state_==AppState::Synthesis){
    ToggleFX(false);
  }
}

/// @brief Scrolls through list of files for user selection
void GrannyChordApp::HandleFileSelection(int32_t encoder_inc){
  int file_count = filemgr_.GetFileCount();
  file_idx_ = (file_idx_ + encoder_inc + file_count) % file_count;
  filemgr_.GetName(file_idx_,fname_);
  DebugPrint(pod_, "selected file %d %s",file_idx_,fname_);
}

/// @brief iterates through synth parameter control modes
void GrannyChordApp::UpdateSynthMode(){
  switch(curr_synth_mode_){
    case SynthMode::Size_Position:
      curr_synth_mode_ = SynthMode::Pitch_ActiveGrains;
      return;
    case SynthMode::Pitch_ActiveGrains:
      curr_synth_mode_ = SynthMode::PhasorMode_EnvType;
      return;
    case SynthMode::PhasorMode_EnvType:
      curr_synth_mode_ = SynthMode::Pan_PanRnd;
      return;
    case SynthMode::Pan_PanRnd:
      curr_synth_mode_ = SynthMode::Size_Position;
      return;
    default:
      return;
  }
}

/// @brief Updates synth parameters based on current synth mode and adjusts
///        knob input values to account for knob jitter and deadzones around 0/1
void GrannyChordApp::UpdateSynthParams(){
  if (curr_state_!=AppState::Synthesis) return;
  int mode_idx = static_cast<int>(curr_synth_mode_);
  float knob1_val = MapKnobDeadzone(pod_.knob1.Process());
  float knob2_val = MapKnobDeadzone(pod_.knob2.Process());

  if (CheckParamDelta(knob1_val,prev_param_vals_k1[mode_idx])){
    UpdateKnob1Params(knob1_val,curr_synth_mode_);
    prev_param_vals_k1[mode_idx] = knob1_val;
  }
  if(CheckParamDelta(knob2_val, prev_param_vals_k2[mode_idx])){
    UpdateKnob2Params(knob2_val, curr_synth_mode_);
    prev_param_vals_k2[mode_idx] = knob2_val;
  }
}

void GrannyChordApp::ToggleRandomnessControls(){
  if (curr_synth_mode_ <= SynthMode::PhasorMode_EnvType_Rnd){
    /* below works because regular modes are even, random modes are odd
      so we can XOR their enum value to get the corresponding mode */
    curr_synth_mode_ = static_cast<SynthMode>(static_cast<int>(curr_synth_mode_) ^ 1);
  }
}

void GrannyChordApp::ToggleFX(bool which_fx){
  if (curr_synth_mode_ >= SynthMode::Reverb){
    curr_synth_mode_ = prev_synth_mode_;
    return;
  }
  prev_synth_mode_ =  curr_synth_mode_;
  /* if true, switch to reverb, else false - cheap but it works */
  curr_synth_mode_ = which_fx ? SynthMode::Reverb : SynthMode::Filter;
}

/// @brief Initialises file manager, sets audio data buffers and scans SD card for WAV files
/// @return True on successful initialisation, false if init fails or no WAV files found
bool GrannyChordApp::InitFileMgr(){
  filemgr_.SetBuffers(left_buf_,right_buf_, temp_buf_);
  if (!filemgr_.Init())return false;
  pod_.seed.PrintLine("scanning wavs now");
  return filemgr_.ScanWavFiles();
}

/// @brief Calls synth initialisation function, passes audio data buffers and audio length
void GrannyChordApp::InitSynth(){
  // synth_.Init(left_buf_, right_buf_, audio_len_);
  synth_.Init(left_buf_, right_buf_, filemgr_.GetSamplesPerChannel());
}

/// @brief Resets current file index and audio data buffers
///        Calls method to handle WAV file selection
void GrannyChordApp::ClearAudioBuffers(){
  memset(left_buf_, 0, CHNL_BUF_SIZE_ABS);
  memset(right_buf_, 0, CHNL_BUF_SIZE_ABS);
}

/// @brief Initialises WAV playback state, resets playhead, sets current file audio length
void GrannyChordApp::InitPlayback(){
  wav_playhead_ = 0;
  DebugPrint(pod_, "loading file");
  // audio_len_ = filemgr_.GetSamplesPerChannel();
  if (!filemgr_.LoadFile(file_idx_)) {
    DebugPrint(pod_,"failed to load file");
    curr_state_=AppState::Error;
    return;
  }
  DebugPrint(pod_, "loaded file");
  pod_.StartAudio(AudioCallback);
  DebugPrint(pod_, "started audio callback");
}

/// @brief Initialises RecordIn state, clears audio buffers
void GrannyChordApp::InitRecordIn(){
  memset(left_buf_, 0, CHNL_BUF_SIZE_ABS);
  memset(right_buf_, 0, CHNL_BUF_SIZE_ABS);

  // DebugPrint(pod_, "cleared audio buffers for recording in");
}

// /// @brief Initialise object for recording out to SD card
// void GrannyChordApp::InitWavWriter(){
//   WavWriter<16384>::Config cfg;
//   cfg.bitspersample = BIT_DEPTH;
//   cfg.channels = filemgr_.GetNumChannels();
//   cfg.samplerate = SAMPLE_RATE;
//   sd_out_writer_.Init(cfg);
// }

/// @brief initialise reverb, compressor, filter configs for FX section 
void GrannyChordApp::InitFX(){
  comp_.Init(pod_.AudioSampleRate());
  reverb_.Init(SAMPLE_RATE_FLOAT);
  lowpass_moog_.Init(SAMPLE_RATE_FLOAT);
  lowpass_moog_.SetFreq(20000.0f);
  lowpass_moog_.SetRes(0.7f);
  hipass_.Init();
  hipass_.SetFilterMode(daisysp::OnePole::FilterMode::FILTER_MODE_HIGH_PASS);
  hipass_.SetFrequency(0.0f); // NOTE: CHECK THIS
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
  instance_->ProcessAudio(in,out,size);
}

void GrannyChordApp::ProcessAudio(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  // loadmeter.OnBlockStart();
  switch(curr_state_){
    case AppState::PlayWAV: 
      if (wav_playhead_>= filemgr_.GetSamplesPerChannel() -1){
        instance_->pod_.StopAudio();
        return;
      }
      ProcessWAVPlayback(out,size);
      return;  
    case AppState::RecordIn:
      ProcessRecordIn(in, out, size);
      return;
    case AppState::Synthesis:
      ProcessSynthesis(out,size);
      return;
    // case AppState::ChordMode:
    default:
      pod_.StopAudio();
      return;
  }
  // loadmeter.OnBlockEnd();
}


// /// @brief Outputs silence if app is in non audio output state
// /// @param out Output audio buffer
// /// @param size Number of samples to process in this call (irrelevant)
// void GrannyChordApp::AudioIdle(AudioHandle::OutputBuffer out, size_t size){
//   for (size_t i=0; i<size; i++){
//     out[0][i]=out[1][i]=0.0f;
//   }
// }

/// @brief Process audio for WAV playback mode and increment playback position
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::ProcessWAVPlayback(AudioHandle::OutputBuffer out, size_t size){
  for (size_t i=0; i<size; i++){
    if (wav_playhead_ < filemgr_.GetSamplesPerChannel()){
      out[0][i] = s162f(left_buf_[wav_playhead_]) * 0.5f;
      out[1][1] = s162f(right_buf_[wav_playhead_]) * 0.5f;
      wav_playhead_++;
    }
  }
  if (wav_playhead_%48000==0){
    pod_.seed.PrintLine("+1s");
  }
}

/// @brief Process input audio when app is in RecordIn state
/// @param in Input audio buffer
/// @param out Output audio buffer
/// @param size Number of samples to process in this call
void GrannyChordApp::ProcessRecordIn(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  const size_t MAX_RECORDING_LEN = 60*48000; /* 60s @ 48kHZ */
  size_t record_pos = 0;
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

    // float temp_left[size];
    // float temp_right[size];
    
    // for (size_t i=0; i<size; i++){
    //   temp_left[i] = out[0][i];
    //   temp_right[i] = out[1][i];
    //   reverb_.Process(temp_left[i],temp_right[i],&out[0][i],&out[1][i]); // NOTE: check this works
    // }
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
// void GrannyChordApp::RecordOutToSD(AudioHandle::OutputBuffer out, size_t size){
//   InitWavWriter();
//   char name[32];
//   sprintf(name,"recording_%d",recording_count_);
//   sd_out_writer_.OpenFile(name);
//   recording_count_++;
//   recording_out_ = true;
// // NOTE: 
// // USE THIS 
// // https://electro-smith.github.io/libDaisy/classdaisy_1_1_wav_writer.html

//   // create file on sd card 
//   // write in blocks]
//   // will have to f2s16 to conv back to int16 
//   // max recording duration should be 120*48000*sizeof(float)*2(channels)??
//   // wrap around index ? or just stop recording at max length
//   // could flash seed led if wrapping, solid if stop at max
//   // write straight to sd card?
//   // or is it better to write to a circular buffer in SDRAM
//   // then asynchronously/more slowly write to SD card
//   // reset recording buffer pointer
//   // set up audio input gain or anything?
//   // init sample counter for recording duration? 
//   // when finished: update file headers
//   // update UI to show recording finished
// }

/// @brief Helper function to mark when audio output recording starts
void GrannyChordApp::ToggleRecordOut(){
  recording_out_ = !recording_out_;
}


void GrannyChordApp::UpdateKnob1Params(float knob1_val, SynthMode mode){
  switch (mode){
    case SynthMode::Size_Position:
      synth_.SetGrainSize(knob1_val);
      return;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetPitchRatio(knob1_val);
      return;
    case SynthMode::Pan_PanRnd:
      synth_.SetPan(knob1_val);
      return;
    case SynthMode::PhasorMode_EnvType:
      knob1_val = fmap(knob1_val, 0, NUM_PHASOR_MODES);
      synth_.SetPhasorMode(static_cast<GrainPhasor::Mode>(knob1_val));
      return;
    case SynthMode::Size_Position_Rnd:
      synth_.SetSizeRnd(knob1_val);
      return;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      synth_.SetPitchRnd(knob1_val);
      return;
    case SynthMode::PhasorMode_EnvType_Rnd:
      synth_.SetPhasorRnd(knob1_val);
      return;
    case SynthMode::Reverb:
      /* set reverb feedback ie tail length */
      reverb_.SetFeedback(knob1_val); // NOTE check it doesn't clip
      return;
    case SynthMode::Filter:
      // knob1_val = fmap(knob1_val, 20.0f, 20000.0f, daisysp::Mapping::EXP);
      /* map knob value to frequency range with an exponential curve */
      knob1_val = fmap(knob1_val, 20.0f, 5000.0f, daisysp::Mapping::EXP);
      /* set cutoff frequency of low pass moog filter */
      lowpass_moog_.SetFreq(knob1_val);
      return;
  }
}

void GrannyChordApp::UpdateKnob2Params(float knob2_val, SynthMode mode){
  switch (mode){
    case SynthMode::Size_Position:
      synth_.SetSpawnPos(knob2_val);
      return;
    case SynthMode::Pitch_ActiveGrains:
      synth_.SetActiveGrains(knob2_val);
      return;
    case SynthMode::Pan_PanRnd:
      synth_.SetPanRnd(knob2_val);
      return;
    case SynthMode::PhasorMode_EnvType:
      knob2_val = fmap(knob2_val, 0, NUM_ENV_TYPES);
      synth_.SetEnvelopeType(static_cast<Grain::EnvelopeType>(knob2_val));
      return;
    case SynthMode::Size_Position_Rnd:
      synth_.SetPositionRnd(knob2_val);
      return;
    case SynthMode::Pitch_ActiveGrains_Rnd:
      synth_.SetCountRnd(knob2_val);
      return;
    case SynthMode::PhasorMode_EnvType_Rnd:
      synth_.SetEnvRnd(knob2_val);
      return;
    case SynthMode::Reverb:
      // /* map knob value to frequency range with an exponential curve */
      // knob2_val = fmap(knob2_val, 100.0f, 20000.0f, daisysp::Mapping::EXP);
      // /* set reverb low pass frequency */
      // reverb_.SetLpFreq(knob2_val);
      reverb_.SetMix(knob2_val);
      return;
    case SynthMode::Filter:
      /* again, map knob value to frequency range with an exponential curve */
      knob2_val = fmap(knob2_val, 0.001f, 0.497f, daisysp::Mapping::EXP);
      /* set cutoff frequency for high pass filter */
      hipass_.SetFrequency(knob2_val);
      return;
  }
}

/* check parameter value change is over a certain level to avoid reading knob jitter */
inline constexpr bool GrannyChordApp::CheckParamDelta(float curr_val, float prev_val){
  return (fabsf(curr_val - prev_val)>0.01f);
}


/* knobs on the Pod have deadzones around 0 and 1 so we adjust for this */
inline constexpr float GrannyChordApp::MapKnobDeadzone(float knob_val ){
  if (knob_val<=0.01f) return 0.0f;
  if (knob_val>=0.99f) return 1.0f;
  return knob_val;
}

/* here we check if the current value of the knob has moved through the last stored value 
  before the synth mode changed, and only update the parameter once the current knob value
  'passes through' the stored value - meaning if we set grain size to 0.1 then switch modes 
  and move the knob to 0.8, if we switch back, grain size won't be updated again until the 
  knob passes through this value.  */

inline constexpr float GrannyChordApp::UpdateKnobPassThru(float curr_knob_val, float *stored_knob_val, bool *pass_thru){
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
      DebugPrint(pod_, "State now in: Error");
        return;
    default:
      DebugPrint(pod_, "default state??");
      return;
  }
};