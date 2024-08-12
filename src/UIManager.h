#pragma once
#include "daisy_pod.h"
#include "AppState.h"

using namespace daisy;

class UIManager {
  public:
    UIManager(DaisyPod& pod): pod_(pod) {}


    AppState GetCurrentState() { return current_state_; }
    SynthMode GetGranularMode() { return synth_mode_; }

    void UpdateControls();
    float GetKnob1Value();
    float GetKnob2Value();
    int32_t GetEncoderIncrement();
    bool EncoderPressed();
    bool EncoderLongPress();
    bool Button1Pressed();
    bool Button2Pressed();
    bool Button2LongPress();

    void SetLed1(int r, int g, int b);
    void BlinkLed1(int r, int g, int b);
    void BlinkSetLed1(int r, int g, int b);


  private:
    DaisyPod& pod_;
    AppState current_state_ = AppState::Startup;
    SynthMode synth_mode_ = SynthMode::Size_Position; 

    static const int NUM_SYNTH_MODES = 7;
    bool k1_pass_thru_[NUM_SYNTH_MODES] = {false};
    bool k2_pass_thru_[NUM_SYNTH_MODES] = {false};
    float k1v_[NUM_SYNTH_MODES] = {0.5f};
    float k2v_[NUM_SYNTH_MODES] = {0.5f};
    // float prev_param_val_[NUM_SYNTH_MODES] = {0.5f};

    float MapKnobDeadzone(float knob_val);
    float UpdateKnobPassThru(float curr_knob_val, float *stored_knob_val, bool *pass_thru);
    void UpdateKnobs();
    // void UpdateEncoder();
    void UpdateState();
    void UpdateSynthMode();
    void ToggleRandomnessControls();



};