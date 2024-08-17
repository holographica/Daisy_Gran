#pragma once 
#include "daisy_pod.h"
#include "daisysp.h"
// #include "constants_utils.h"
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
    SynthMode curr_synth_mode_;
    SynthMode prev_synth_mode_;
    
    bool changing_state_ = false;

    /* audio FX and filters */
    Compressor comp_;
    ReverbSc reverb_;
    MoogLadder lowpass_moog_;
    OnePole hipass_;

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

};