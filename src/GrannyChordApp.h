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
    const float PARAM_CHANGE_THRESHOLD = 0.01f;
    float prev_grain_size = 0.5f;
    float prev_pos = 0.5f;
    float prev_active_count = 0.5f;
    float prev_pitch = 0.5f;


    // initialise file manager, ui manager, do startup stuff
    // get updated state from ui manager
    bool InitFileMgr();
    void InitSynth();
    void InitCompressor();
    // handle file selection, playback, chord mode

    bool HandleFileSelection();

    void HandleGranulation();
    void UpdateSynthParams();
    bool CheckParamDelta(float curr_val, float prev_val);

    // what else? need to update controls and states



};