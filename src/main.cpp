#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "AudioFileManager.h"
#include "GranularSynth.h"
#include "constants.h"
#include "../DaisySP/DaisySP-LGPL/Source/Dynamics/compressor.cpp"

using namespace daisy;
using namespace daisysp;
using namespace std;

SdmmcHandler sd;
FatFSInterface fsi;
DaisyPod pod;
FIL file;
AudioFileManager filemgr(sd, fsi, pod, &file);


DSY_SDRAM_BSS alignas(16) int16_t left_buf[CHNL_BUF_SIZE_SAMPS];
DSY_SDRAM_BSS alignas(16) int16_t right_buf[CHNL_BUF_SIZE_SAMPS];

GranularSynth synth(pod);

Compressor comp;
Limiter lim;
size_t audio_len=0;

/* track previous param values (start as defaults) */
float prev_grain_size = 0.5f;
float prev_pos = 0.5f;
float prev_active_count = 0.5f;
float prev_pitch = 0.5f;
const float PARAM_CHANGE_THRESHOLD = 0.01f;

bool k1_pass_thru_mode0 = false;
bool k1_pass_thru_mode1 = false;
bool k2_pass_thru_mode0 = false;
bool k2_pass_thru_mode1 = false;
float k1v_mode_0 = 0.5f;
float k1v_mode_1 = 0.5f;
float k2v_mode_0 = 0.5f;
float k2v_mode_1 = 0.5f;

uint16_t file_idx = 1;
int mode = 0;



/* TODO here:
- set up error message function so i can turn debug mode on/off with a bool
- then change all pod prints to error msg so only prints if in debug mode
*/

// void SetLed1(int r, int g, int b){
//   pod.led1.Set(r,g,b);
//   pod.UpdateLeds();
//   System::Delay(100);
// }

// void PulseLed1(){
//   pod.led1.Set(0,0,0);
//   pod.UpdateLeds();
//   int cnt = 0;
//   while(cnt > 255){
//     cnt++;
//     pod.led1.Set(cnt,0,0);
//     System::Delay(10);
//   }
//   while(cnt>=0){
//     cnt--;
//     pod.led1.Set(cnt,0,0);
//     System::Delay(10);
//   }
// }


// void BlinkLed1(int r, int g, int b){
//   SetLed1(r,g,b);
//   SetLed1(0,0,0);
// }

// void BlinkSetLed1(int r, int g, int b){
//   BlinkLed1(r,g,b);
//   BlinkLed1(r,g,b);
//   SetLed1(r,g,b);
// }

// void SetLed1Green(){
//   SetLed1(0,255,0);
// }

// void SetLed1Blue(){
//   SetLed1(0,0,255);
// }

// void BlinkLed1White(){
//   BlinkLed1(255,255,255);
//   BlinkLed1(255,255,255);
// }

// void BlinkLed1Green(){
//   BlinkLed1(0,255,0);
//   BlinkLed1(0,255,0);
// }

// void BlinkLed1Blue(){
//   BlinkLed1(0,0,255);
//   BlinkLed1(0,0,255);
// }



/* knobs on the Pod have a small deadzone around the upper/lower bounds
  (eg my knob1 only goes down to 0.003) -> assume knob is at 0 or 1 if it's very close */
// float MapKnobDeadzone(float knob_val){
//   if (knob_val<=0.01f) { knob_val = 0.0f; }
//   else if (knob_val>=0.99f) { knob_val = 1.0f; }
//   return knob_val;
// }

// float UpdateKnobPassThru(float curr_knob_val, float *stored_knob_val, float prev_param_val, bool *pass_thru){
//   if (!(*pass_thru)){
//     if ((curr_knob_val >= prev_param_val && (*stored_knob_val) <= (prev_param_val)) ||
//         (curr_knob_val <= prev_param_val && (*stored_knob_val) >= (prev_param_val))) {
//           (*pass_thru) = true;
//     }
// } 
//   if (*pass_thru){
//     (*stored_knob_val) = curr_knob_val;
//     return curr_knob_val;
//   }
//   return prev_param_val;
// }

// bool CheckParamDelta(float curr_val, float prev_val){
//   return (fabsf(curr_val - prev_val)>0.01f);
// }

void UpdateKnob1(int mode){
  float k1v = MapKnobDeadzone(pod.knob1.Process());
  if (mode==0){
    // use k1 to control grain size (10ms to 1s)
    float grain_size = UpdateKnobPassThru(k1v, &k1v_mode_0, prev_grain_size, &k1_pass_thru_mode0);
    if (CheckParamDelta(grain_size, prev_grain_size)){
      synth.SetUserGrainSize(grain_size);
      prev_grain_size = grain_size;
      pod.seed.PrintLine("grain size set to %.3f ms", grain_size);
    }
  }
  else if (mode ==1){
    float pitch_ratio = UpdateKnobPassThru(k1v, &k1v_mode_1, prev_pitch, &k1_pass_thru_mode0);
    if (CheckParamDelta(pitch_ratio, prev_pitch)){
      synth.SetUserPitchRatio(pitch_ratio);
      prev_pitch= pitch_ratio;
      pod.seed.PrintLine("pitch ratio set to %.3f",pitch_ratio);
    }
  }
}

void UpdateKnob2(int mode){
  float k2v = MapKnobDeadzone(pod.knob2.Process());
  if (mode==0){
    float spawn_pos = UpdateKnobPassThru(k2v, &k2v_mode_0, prev_pos, &k2_pass_thru_mode0);
    if (CheckParamDelta(spawn_pos, prev_pos)){
      synth.SetUserSpawnPos(spawn_pos);
      prev_pos = spawn_pos;
      pod.seed.PrintLine("spawn pos set to %.3f", spawn_pos);
    }
  }
  else if (mode==1){
    float active_count = UpdateKnobPassThru(k2v, &k2v_mode_1, prev_active_count, &k2_pass_thru_mode0);
    if (CheckParamDelta(active_count, prev_active_count)){
      synth.SetUserActiveGrains(active_count);
      prev_active_count = active_count;
      pod.seed.PrintLine("active grains set to %.3f",active_count);
    }
  }
}

// void InitSynth(){
//   audio_len = filemgr.GetSamplesPerChannel();
//   pod.seed.PrintLine("File loaded: %d samples", audio_len);
//   synth.SetUserGrainSize(prev_grain_size);
//   synth.SetUserSpawnPos(prev_pos);
//   synth.SetActiveGrains(1);
//   synth.Init(left_buf, right_buf, audio_len);
// }


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  synth.ProcessGrains(out[0], out[1], size);
  comp.ProcessBlock(out[0],out[0], size);
  comp.ProcessBlock(out[1],out[1], size);
}


// void UpdateEncoder(){
//   pod.encoder.Debounce();
//   if (pod.encoder.TimeHeldMs()>1000){
//     if (stopped){
//       pod.StartAudio(AudioCallback);
//       stopped = false;
//     }
//     else if (!stopped){
//       pod.StopAudio();
//       stopped = true;
//     }
//   }

//   int32_t inc = pod.encoder.Increment();
//   if (inc!=0){
//     char fname[64];
//     file_idx+=inc;
//     if (file_idx<0) { file_idx = filemgr.GetFileCount() - 1; }
//     if (file_idx >= filemgr.GetFileCount()) { file_idx = 0; }
//     filemgr.GetName(file_idx, fname);
//     pod.seed.PrintLine("selected new file idx %d %s", file_idx, fname);
//   }
//   if (pod.encoder.FallingEdge()){
//     if (filemgr.LoadFile(file_idx)) {
//       InitSynth();
//     } 
//     else {
//       // TODO !!! same for other errors
//       // NOTE: change state to AppState::Error here !!!! 
//       pod.seed.PrintLine("Failed to load audio file");
//       return;
//     }
//   }
// }

void UpdateControls() {
  pod.button2.Debounce();
  if (pod.button2.FallingEdge()){
    mode++;
    /* wrap around mode value */
    if (mode>1) { mode = 0; }
    if (mode==0){
      k1v_mode_0 = MapKnobDeadzone(pod.knob1.GetRawFloat());
      k2v_mode_0 = MapKnobDeadzone(pod.knob2.GetRawFloat());
    }
    if (mode==1){
      k1v_mode_1 = MapKnobDeadzone(pod.knob1.GetRawFloat());
      k2v_mode_1 = MapKnobDeadzone(pod.knob2.GetRawFloat());
    }
    k1_pass_thru_mode0 = false;
    k1_pass_thru_mode1 = false;
    k2_pass_thru_mode0 = false;
    k2_pass_thru_mode1 = false;
    mode==0 ? BlinkSetLed1(0,255,0) : BlinkSetLed1(0,0,255);
  }
  UpdateKnob1(mode);
  UpdateKnob2(mode);
  UpdateEncoder();
}

// void InitCompressor(){
//   comp.Init(pod.AudioSampleRate());
//   comp.SetRatio(3.0f);
//   comp.SetAttack(0.01f);
//   comp.SetRelease(0.1f);
//   comp.SetThreshold(-12.0f);
//   comp.AutoMakeup(true);
// }







int main (void){
  pod.Init();
  pod.seed.StartLog(true);
  pod.SetAudioBlockSize(4);
  pod.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  InitCompressor();

  PulseLed1();
  PulseLed1();
  // BlinkLed1White();

  filemgr.SetBuffers(left_buf, right_buf);
  if (!filemgr.Init()){
    // TODO
    // NOTE: SET STATE TO ERROR
    pod.seed.PrintLine("filemgr init failed");
    return 1;
  }
  BlinkLed1White();
  if (!filemgr.ScanWavFiles()){
    // TODO
    // NOTE: SET STATE TO ERROR
    pod.seed.PrintLine("reading files failed");
    return 1;
  }
  BlinkLed1White();
  if (filemgr.LoadFile(6)) {
    InitSynth();
    // PulseLed1();
    // PulseLed1();
    // BlinkLed1White();
  } 
  else {
    // TODO 
    // NOTE: SET STATE TO ERROR
    pod.seed.PrintLine("Failed to load audio file");
    return 1;
  }

  pod.StartAdc();
  pod.StartAudio(AudioCallback);

  while(1){    
    UpdateControls();
    // System::Delay(10);
  }
}




// let grains through gate when button pressed 
// use button to trigger grain instead of automatically generating it 
// 


// have a rng that chooses which order the notes of the chord are triggered
// could have smal chance of playing random note
// press button to choose random scale

