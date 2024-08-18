#pragma once 
#include "daisy_pod.h"
#include "AppState.h"
#include "daisysp.h"
#include "constants_utils.h"
#include "GranularSynth.h"
#include "AudioFileManager.h"
#include "debug_print.h"
#include "DaisySP-LGPL-FX/compressor.h"
#include "DaisySP-LGPL-FX/reverbsc.h"
#include "DaisySP-LGPL-FX/moogladder.h"

using namespace daisy;
using namespace daisysp;

class GrannyChordApp {
  public:
  // GrannyChordApp(DaisyPod& pod, GranularSynth& synth, AudioFileManager& filemgr)
  GrannyChordApp(DaisyPod& pod, GranularSynth& synth, AudioFileManager& filemgr, ReverbSc& reverb)
    // : pod_(pod), synth_(synth), filemgr_(filemgr){
    : pod_(pod), synth_(synth), filemgr_(filemgr), reverb_(reverb){
      instance_ = this;
    };

    void Init(int16_t *left, int16_t *right);
    void Run();

    // void GetLoadmeter(CpuLoadMeter* meter) { loadmeter = meter; }
    CpuLoadMeter loadmeter;


    static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    static GrannyChordApp* instance_;

  private:
    DaisyPod &pod_;
    GranularSynth synth_;
    AudioFileManager &filemgr_;
    FIL *file_;

    /* UI and state objects */
    AppState curr_state_;
    SynthMode curr_synth_mode_;
    SynthMode prev_synth_mode_;
    
    /* audio FX and filters */
    ReverbSc& reverb_;
    Compressor comp_;
    MoogLadder lowpass_moog_;
    OnePole hipass_;

    /* audio data channel buffers */
    int16_t *left_buf_;
    int16_t *right_buf_;

    int file_idx_;
    size_t wav_playhead_;
    uint32_t audio_len_;
    char fname_[MAX_FNAME_LEN];

    /* previous values for parameters controlled by knob 1*/
    float prev_param_vals_k1[NUM_SYNTH_MODES];
    float prev_param_vals_k2[NUM_SYNTH_MODES];

    bool changing_state_ = false;
    bool recording_out_ = false;
    WavWriter<16384> sd_out_writer_;
    int recording_count_;

    void UpdateUI();
    void UpdateSynthMode();

    /* audio input/output/recording methods based on state */
    void ProcessAudio(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    void AudioIdle(AudioHandle::OutputBuffer out, size_t size);
    void ProcessWAVPlayback(AudioHandle::OutputBuffer out, size_t size);
    void ProcessRecordIn(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    void ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size);
    void ProcessChordMode(AudioHandle::OutputBuffer out, size_t size);
    void RecordOutToSD(AudioHandle::OutputBuffer out, size_t size);
    // void ToggleRecordOut();

    void HandleEncoderPressed();
    void HandleEncoderLongPress();
    void HandleButton1();
    void HandleButton2();
    void HandleButton1LongPress();
    void HandleButton2LongPress();
    void ToggleRandomnessControls();
    void ToggleFX(bool which_fx);

    /* methods to init / prepare for state change */
    void ClearAudioBuffers();
    bool InitFileMgr();
    void InitPlayback();
    void InitSynth();
    void InitFX();
    void InitRecordIn();
    void InitWavWriter();
    void InitPrevParamVals();

    void HandleStateChange();
    void HandleFileSelection(int32_t encoder_inc);

    /* methods to update/init synth parameters */
    void UpdateSynthParams();
    void UpdateKnob1Params(float knob1_val, SynthMode mode);
    void UpdateKnob2Params(float knob2_val, SynthMode mode);

    inline constexpr bool CheckParamDelta(float curr_val, float prev_val);
    inline constexpr float MapKnobDeadzone(float knob_val);
    inline constexpr float UpdateKnobPassThru(float curr_knob_val, float *stored_knob_val, bool *pass_thru);

    void DebugPrintState(AppState state);

};