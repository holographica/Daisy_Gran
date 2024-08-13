#pragma once 
#include "daisy_pod.h"
#include "daisysp.h"
#include "UIManager.h"
#include "GranularSynth.h"
#include "AudioFileManager.h"

#include "../DaisySP/DaisySP-LGPL/Source/Dynamics/compressor.cpp"


using namespace daisy; 

class GrannyChordApp {
  public:
  GrannyChordApp(DaisyPod& pod, GranularSynth& synth, AudioFileManager& filemgr)
        : pod_(pod), synth_(synth), 
          filemgr_(filemgr), ui_(pod) {};

    void Init();
    void Run();
    void AudioCallback(AudioHandle::AudioCallback callback);
    // void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);

  private:
    DaisyPod &pod_;
    GranularSynth &synth_;
    AudioFileManager &filemgr_;
    FIL *file_;
    UIManager ui_;

    Compressor comp;

    float file_idx_ = 0; 
    // AppState curr_state_;
    // const float PARAM_CHANGE_THRESHOLD = 0.01f;
    float prev_grain_size_ = 0.5f;
    float prev_pos_ = 0.5f;
    float prev_active_count_ = 0.5f;
    float prev_pitch_ = 0.5f;
    /* previous values for parameters controlled by knob 1*/
    float prev_param_vals_k1[NUM_SYNTH_MODES];
    float prev_param_vals_k2[NUM_SYNTH_MODES];


    // initialise file manager, ui manager, do startup stuff
    // get updated state from ui manager
    bool InitFileMgr();
    void InitSynth();
    void InitCompressor();
    // handle file selection, playback, chord mode

    // void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);

    bool HandleFileSelection();
    void HandleGranulation();

    void UpdateSynthParams();
    void UpdateGranularParams();
    bool CheckParamDelta(float curr_val, float prev_val);

    void InitPrevParamVals(){
      for (int i=0; i < NUM_SYNTH_MODES;i++){
        prev_param_vals_k1[i] = 0.5f;
        prev_param_vals_k2[i] = 0.5f;
      } 
    }

    // what else? need to update controls and states



};