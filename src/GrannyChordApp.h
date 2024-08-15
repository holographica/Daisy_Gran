#pragma once 
#include "daisy_pod.h"
#include "daisysp.h"
#include "UIManager.h"
#include "GranularSynth.h"
#include "AudioFileManager.h"
#include "debug_print.h"

using namespace daisy;
using namespace daisysp;

class GrannyChordApp {
  public:
  GrannyChordApp(DaisyPod& pod, GranularSynth& synth, AudioFileManager& filemgr)
        : pod_(pod), synth_(synth), 
          filemgr_(filemgr), ui_(pod), left_buf_(nullptr), right_buf_(nullptr) {
            instance_ = this;
          };

    void Init(int16_t *left, int16_t *right);
    void Run();

    static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    static GrannyChordApp* instance_;

  private:
    DaisyPod &pod_;
    GranularSynth synth_;
    AudioFileManager &filemgr_;
    FIL *file_;
    UIManager ui_;
    Compressor comp_;

    int16_t *left_buf_;
    int16_t *right_buf_;

    int file_idx_ = 0;
    size_t wav_playhead_ = 0;
    uint32_t audio_len_ = 0;
    // AppState curr_app_state_;
    /* previous values for parameters controlled by knob 1*/
    float prev_param_vals_k1[NUM_SYNTH_MODES];
    float prev_param_vals_k2[NUM_SYNTH_MODES];

    bool recording_out_ = false;

    /* audio input/output/recording methods based on state */
    void AudioIdle(AudioHandle::OutputBuffer out, size_t size);
    void ProcessWAVPlayback(AudioHandle::OutputBuffer out, size_t size);
    void ProcessRecordIn(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    void ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size);
    void ProcessChordMode(AudioHandle::OutputBuffer out, size_t size);
    void RecordOutToSD(AudioHandle::OutputBuffer out, size_t size);
    void StartRecordOut();
    void StopRecordOut();

    /* methods to init / prepare for state change */
    bool InitFileMgr();
    void InitPlayback();
    void InitSynth();
    void InitCompressor();
    void InitFileSelection();
    void InitRecordIn();

    void HandleStateChange(AppState prev, AppState curr);
    bool HandleFileSelection();

    /* methods to update/init synth parameters */
    void UpdateSynthParams(AppState curr_state);
    void UpdateGranularParams();
    // void UpdateChordParams();
    bool CheckParamDelta(float curr_val, float prev_val);
    void InitPrevParamVals();
};