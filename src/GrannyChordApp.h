#pragma once 
#include "daisy_pod.h"
#include "daisysp.h"
#include "GranularSynth.h"
#include "AudioFileManager.h"
#include "constants_utils.h"
#include "debug_print.h"
#include "DaisySP-LGPL-FX/reverb.h"
#include "DaisySP-LGPL-FX/compressor.h"
#include "DaisySP-LGPL-FX/moogladder.h"
#include "AppState.h"

using namespace daisy;
using namespace daisysp;

class GrannyChordApp {
  public:
  GrannyChordApp(DaisyPod& pod, GranularSynth& synth, AudioFileManager& filemgr, ReverbSc &reverb)
        : pod_(pod), synth_(synth), 
          filemgr_(filemgr), reverb_(reverb){
            instance_ = this;
          };

    void Init(int16_t *left, int16_t *right, int16_t *temp);
    void Run();

    CpuLoadMeter loadmeter;

    static void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    void ProcessAudio(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    static GrannyChordApp* instance_;

  private:
    DaisyPod &pod_;
    GranularSynth synth_;
    AudioFileManager &filemgr_;
    FIL *file_;

    /* UI and state objects */
    AppState curr_state_;
    AppState next_state_;
    SynthMode curr_synth_mode_;
    SynthMode prev_synth_mode_;

    /* audio FX and filters */
    Compressor comp_;
    ReverbSc& reverb_;
    MoogLadder lowpass_moog_;
    OnePole hipass_;

    /* audio data channel buffers */
    int16_t *left_buf_;
    int16_t *right_buf_;

    int16_t *temp_buf_;

    int file_idx_ = 0;
    size_t wav_playhead_ = 0;
    uint32_t audio_len_ = 0;
    char fname_[MAX_FNAME_LEN];

    /* previous values for parameters controlled by knob 1*/
    float prev_param_vals_k1[NUM_SYNTH_MODES];
    float prev_param_vals_k2[NUM_SYNTH_MODES];
    WavWriter<16384> sd_writer_;

    
    bool recorded_in_ = false;
    size_t record_in_pos_ = 0;
    bool recording_out_ = false;
    size_t recording_count_ = 0;
    size_t loop_count=0;
    float temp_interleaved_buf_[2];

    TimerHandle timer_;
    float led1_rgb_[3] = {0};
    float led2_rgb_[3] = {0};
    int led_flash_count_=0;


    void UpdateUI();
    void UpdateSynthMode();

    /* audio input/output/recording methods based on state */
    void ProcessWAVPlayback(AudioHandle::OutputBuffer out, size_t size);
    void ProcessRecordIn(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    void ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size);
    void ProcessChordMode(AudioHandle::OutputBuffer out, size_t size);
    void RecordOutToSD(AudioHandle::OutputBuffer out, size_t size);
    void ToggleRecordOut();
    void FinishRecording();

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
    // void UpdateChordParams();
    inline constexpr bool CheckParamDelta(float curr_val, float prev_val);
    inline constexpr float MapKnobDeadzone(float knob_val);
    inline constexpr float UpdateKnobPassThru(float curr_knob_val, float *stored_knob_val, bool *pass_thru);

    void DebugPrintState(AppState state);
    void DebugPrintMode(SynthMode mode);

    /* has to be static - timer won't take class member function in callback */
    static void StaticLedCallback(void* data){
      instance_ -> LedCallback(data);
    }

    void SetupTimer();
    // void StartLeds();
    void LedCallback(void* data);
    void SetLedAppState();
    void SetLedSynthMode();
    void SetLedRandomMode();
    void SetLedFXMode();
    void SetRgb1(float r, float g, float b);
    void SetRgb2(float r, float g, float b);


};