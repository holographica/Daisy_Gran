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

    /* UI and state objects */
    UIManager ui_;
    AppState curr_state_;
    AppState prev_state_;
    bool changing_state_ = false;

    /* audio FX */
    Compressor comp_;
    ReverbSc reverb_;
    
    /* audio filters */
    MoogLadder lowpass_moog_;
    OnePole hipass_onepole_;

    /* audio data channel buffers */
    int16_t *left_buf_;
    int16_t *right_buf_;

    int file_idx_ = 0;
    char fname[MAX_FNAME_LEN];
    size_t wav_playhead_ = 0;
    uint32_t audio_len_ = 0;

    /* previous values for parameters controlled by knob 1*/
    float prev_param_vals_k1[NUM_SYNTH_MODES];
    float prev_param_vals_k2[NUM_SYNTH_MODES];

    bool recording_out_ = false;
    WavWriter<16384> sd_out_writer_;
    int recording_count_;
    char recording_fname_[16];

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
    void InitWavWriter();

    void RequestStateChange(AppState next_state);
    void HandleStateChange();
    void HandleFileSelection();

    /* methods to update/init synth parameters */
    void UpdateSynthParams();
    void UpdateGranularParams();
    void UpdateKnob1Params(float knob1_val, SynthMode mode);
    void UpdateKnob2Params(float knob2_val, SynthMode mode);
    // void UpdateChordParams();
    // void UpdateFXParams();
    bool CheckParamDelta(float curr_val, float prev_val);
    void InitReverb();
    void InitFilters();
    void InitPrevParamVals();

};