#pragma once
#include "daisy_pod.h"
#include "AppState.h"
#include "constants_utils.h"

using namespace daisy;

class UIManager {
  public:
    UIManager(DaisyPod& pod): pod_(pod) {}

    void Init();

    /* methods relating to states and modes of the app and UI */
    AppState GetCurrentState() { return current_state_; }
    SynthMode GetSynthMode() { return synth_mode_; }
    void SetStateError();
    void SetState(AppState state);
    void UpdateUI();
    bool ToggleRecordOut();

    /* hardware input helper methods */
    bool EncoderPressed();
    bool EncoderLongPress();
    int32_t GetEncoderIncrement();
    bool Button1Pressed();
    bool Button2Pressed();
    bool Button2LongPress();
    float GetKnob1Value(int mode_idx);
    float GetKnob2Value(int mode_idx);
    void LedPulseCallback();


  private:
    DaisyPod& pod_;
    AppState current_state_ = AppState::Startup;
    SynthMode synth_mode_ = SynthMode::Size_Position;
    TimerHandle timer_;

    /* track if LED brightness is increasing
      or decreasing for LED pulse callback */
    bool pulse_increasing_;
    uint8_t pulse_brightness_;
    uint8_t pulse_count_=0;
    static const uint8_t PULSE_STEP = 5;

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

    /* LED callback, update and color-setting methods */
    void SetupTimer();
    void StartLedPulse();
    void StopLedPulse();
    void SetLedRandomMode();
    void SetLedSynthMode();
    void SetLed(int r, int g, int b, bool is_Led1);
    void BlinkLed(int r, int g, int b, bool is_Led1);
    void BlinkSetLed(int r, int g, int b, bool is_Led1);



};