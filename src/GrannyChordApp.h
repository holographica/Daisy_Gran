#pragma once 
#include "daisy_pod.h"
#include "daisysp.h"
#include "GranularSynth.h"
#include "AudioFileManager.h"
#include "debug_print.h"
#include "DaisySP-LGPL-FX/compressor.h"
#include "DaisySP-LGPL-FX/moogladder.h"
#include "StereoRotator.h"
#include "AppState.h"

using namespace daisy;
using namespace daisysp;

class GrannyChordApp {
  public:
  GrannyChordApp(DaisyPod& pod, GranularSynth& synth, AudioFileManager& filemgr,\
                ReverbSc &reverb)
        : pod_(pod), synth_(synth), 
          filemgr_(filemgr), reverb_(reverb){
            instance_ = this;
          };

    void Init(int16_t *left, int16_t *right);
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
    ChordMode chord_gen_;

    /* UI and state objects */
    AppState curr_state_;
    AppState next_state_;
    SynthMode curr_synth_mode_;
    SynthMode prev_synth_mode_;
    bool knob1_latched;
    bool knob2_latched;


    /* audio FX and filters */
    Limiter limiter_;
    ReverbSc& reverb_;
    MoogLadder lowpass_moog_;
    OnePole hipass_;
    StereoRotator rotator_;
    /* filter to reduce high end noise */
    OnePole hicut_;

    /* audio data channel buffers */
    int16_t *left_buf_;
    int16_t *right_buf_;

    int file_idx_ = 0;
    size_t wav_playhead_ = 0;
    uint32_t audio_len_ = 0;
    char fname_[MAX_FNAME_LEN];

    /* previous values for parameters controlled by knob 1*/
    float prev_param_k1[NUM_SYNTH_MODES];
    float prev_param_k2[NUM_SYNTH_MODES];

    float prev_k1_pos[NUM_SYNTH_MODES];
    float prev_k2_pos[NUM_SYNTH_MODES];

    /* objects/variables for recording in and out */
    WavWriter<16384> sd_writer_;
    bool recorded_in_ = false;
    size_t record_in_pos_ = 0;
    bool recording_out_ = false;
    size_t recording_count_ = 0;
    size_t loop_count=0;
    float temp_interleaved_buf_[2];

    struct Colours{
      Color BLUE;
      Color GREEN;
      Color RED;
      Color CYAN;
      Color PURPLE;
      Color ORANGE;
      Color YELLOW;
      Color PINK;
      Color OFF;
    };
    Colours colours;
    bool seed_led_state_ = false;
    bool file_led_state_ = true;

    /* methods to init / prepare for state change */
    bool InitFileMgr();
    void InitPlayback();
    void InitSynth();
    void InitFX();
    void InitRecordIn();
    void InitWavWriter();
    void InitPrevParamVals();
    // void ResetPassThru();

    /* state change handlers */
    void UpdateUI();
    void NextSynthMode();
    void PrevSynthMode();
    void HandleStateChange();
    void HandleFileSelection(int32_t encoder_inc);

    bool UserTriggeredChord();

    /* audio input/output/recording methods based on state */
    void ProcessWAVPlayback(AudioHandle::OutputBuffer out, size_t size);
    void ProcessRecordIn(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
    void ProcessSynthesis(AudioHandle::OutputBuffer out, size_t size, bool process_chord);
    Sample ProcessFX(Sample in);
    // void ProcessChordMode(AudioHandle::OutputBuffer out, size_t size);
    void RecordOutToSD();
    void FinishRecording();

    /* hardware input handler methods */
    void ButtonHandler();
    void HandleEncoderIncrement(int encoder_inc);
    void HandleEncoderPressed();
    void HandleEncoderLongPress();
    void HandleButton1();
    void HandleButton2();
    void HandleButton1LongPress();
    void UpdateParams();

    /* methods to update synth parameters */
    void UpdateSynthParams();
    void UpdateKnob1SynthParams(float knob1_val, SynthMode mode);
    void UpdateKnob2SynthParams(float knob2_val, SynthMode mode);

    /* methods to update chord parameters */

    void CycleChordPlaybackMode();
    void CycleChordScale();
    void ChangeChordKey();
    void ChangeChordSpawnPos();

    inline float MapKnobDeadzone(float knob_val);

    void SetLedAppState();
    void SetLedSynthMode();
    void SetLedChordMode();
    void InitColours();

    void DebugPrintState(AppState state);
    void DebugPrintMode(SynthMode mode);
};