#pragma once
#include "daisy_pod.h"
#include "AppState.h"
#include "constants_utils.h"
#include "debug_print.h"

using namespace daisy;

class UIManager {
  public:
    UIManager(DaisyPod& pod): pod_(pod) {}

    void Init();

    /* methods relating to states and modes of the app and UI */
    AppState GetCurrentState() { return current_state_; }
    SynthMode GetCurrentSynthMode() { return current_synth_mode_; }
    void SetStateError();
    void SetState(AppState state);
    void UpdateUI();
    /* hardware input handler methods */
    void HandleButton1();
    void HandleButton2();
    void HandleButton1LongPress();
    void HandleButton2LongPress();
    void HandleEncoderPressed();
    void HandleEncoderLongPress();
    bool ToggleRecordOut();

    /* helper methods to get hardware input control values  */
    bool EncoderPressed();
    bool EncoderLongPress();
    int32_t GetEncoderIncrement();
    bool Button1Pressed();
    bool Button1LongPress();
    bool Button2Pressed();
    bool Button2LongPress();
    float GetKnob1Value(int mode_idx);
    float GetKnob2Value(int mode_idx);
    void LedCallback();
    
    void DebugPrintState();
    void DebugPrintSynthMode();

  private:
    DaisyPod& pod_;
    AppState current_state_;
    SynthMode current_synth_mode_ = SynthMode::Size_Position;
    TimerHandle timer_;
    SynthMode prev_synth_mode_;


    uint8_t led_brightness_;
    bool led_state_ = false;
    daisy::Color led_colours_[8];

    bool crash_error = false;

    /* Stores current and previous values of hardware knobs 
      to implement 'pass-thru' mode - if synth mode changes, the
      parameter that the knob was previously controlling (eg grain size)
      won't be updated until 1) the synth is in that mode again,
      and 2) the knob passes through that value again */
    bool k1_pass_thru_[NUM_SYNTH_MODES] = {false};
    bool k2_pass_thru_[NUM_SYNTH_MODES] = {false};
    float k1v_[NUM_SYNTH_MODES] = {0.5f};
    float k2v_[NUM_SYNTH_MODES] = {0.5f};

    void UpdateState();

    /* Methods relating to synth parameters and hardware input */
    void UpdateKnobs();
    float MapKnobDeadzone(float knob_val);
    float UpdateKnobPassThru(float curr_knob_val, float *stored_knob_val, bool *pass_thru);
    void UpdateSynthMode();
    void ToggleRandomnessControls();
    void ToggleReverbControls();
    void ToggleFilterControls();

    /* LED callback, update and color-setting methods */

    void SetupTimer();
    void StartLedPulse();
    void StopLedPulse();
    void SetLedRandomMode();
    void SetLedSynthMode();
    void SetLedFXMode();
    void SetLed(int r, int g, int b, bool is_Led1);
    void BlinkLed(int r, int g, int b, bool is_Led1);
    void BlinkSetLed(int r, int g, int b, bool is_Led1);



};