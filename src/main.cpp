#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "AudioFileManager.h"
#include "constants.h"

using namespace daisy;
using namespace daisysp;

// constexpr size_t OUTPUT_BUFFER_SIZE = 48000*10 // 10seconds @ 48kHz

SdmmcHandler sd;
FatFSInterface fsi;
DaisyPod pod;
FIL file;
AudioFileManager filemgr(&sd, &fsi, &pod, &file);

DSY_SDRAM_BSS alignas(16) int16_t left_buf[CHNL_BUF_SIZE_SAMPS];
DSY_SDRAM_BSS alignas(16) int16_t right_buf[CHNL_BUF_SIZE_SAMPS];

size_t wav_pos = 0;
bool change_file = false;
bool now_playing = false;
int curr_file_idx = 0;
uint32_t numsamples = 0; // NOTE: is this used for anything? 

Color led1_colour;
Color led2_colour;
float k1v =0.0f;
float k2v =0.0f;



/* TODO here:
- set up error message function so i can turn debug mode on/off with a bool
- then change all pod prints to error msg so only prints if in debug mode
*/

int16_t GetLeftBufData(size_t pos) {
  return (pos < CHNL_BUF_SIZE_SAMPS) ? left_buf[pos] : 0;
}

int16_t GetRightBufData(size_t pos) {
  return (pos < CHNL_BUF_SIZE_SAMPS) ? right_buf[pos] : 0;
}

void SetLedColours(Color::PresetColor color1, Color::PresetColor color2){
  // pod.ClearLeds();
  led1_colour.Init(color1);
  led2_colour.Init(color2);
  pod.led1.SetColor(led1_colour);
  pod.led2.SetColor(led2_colour);
  pod.UpdateLeds();
}

void LoadNewFile(){
  memset(left_buf, 0, sizeof(left_buf));
  memset(right_buf, 0, sizeof(right_buf));
  pod.ClearLeds();
  pod.UpdateLeds();
  filemgr.LoadFile(curr_file_idx);
  numsamples = filemgr.GetSamplesPerChannel();
  wav_pos = 0;
  change_file = false;
  pod.seed.PrintLine("NOW PLAYING: %d", curr_file_idx);
}

void HandleEncoder(){
  pod.encoder.Debounce();
  int32_t inc = pod.encoder.Increment();
  char fname[128];
  if (inc!=0){
    now_playing = false;
    curr_file_idx+=inc;
    if (curr_file_idx<0) { curr_file_idx = filemgr.GetFileCount() - 1; }
    if (curr_file_idx >= filemgr.GetFileCount()) { curr_file_idx = 0; }
    filemgr.GetName(curr_file_idx, fname);
    pod.seed.PrintLine("selected new file idx %d %s", curr_file_idx, fname);
  }
}

void HandleButton1(){
  pod.button1.Debounce();
  if (pod.button1.RisingEdge()){
      change_file = true;
      now_playing = false;
      LoadNewFile();
    }
}

void HandleButton2(){
  pod.button2.Debounce();
  if (pod.button2.RisingEdge() && !change_file){
    if (now_playing) { pod.seed.PrintLine("pause"); }
    if (!now_playing) { pod.seed.PrintLine("play"); }
    now_playing = !now_playing;
  }
}

void HandleKnobs(){
  float k1f = pod.knob1.GetRawFloat();
  float k2f = pod.knob2.GetRawFloat();
  float k1diff = fabsf(k1f-k1v);  
  float k2diff = fabsf(k2f-k2v);  
  if (k1diff > 0.002 || k2diff > 0.002 ){
    pod.seed.PrintLine("knob floats: %.3f | %.3f", k1f,k2f);
    k1v = k1f;
    k2v = k2f;
  }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  static uint32_t callback_count = 0;
  static uint32_t last_report = 0;

  if (wav_pos >= CHNL_BUF_SIZE_SAMPS) {
    pod.seed.PrintLine("Error: wav_pos buffer overrun detected");
    wav_pos = 0;
  }
  if (!now_playing){
    for (size_t i=0; i<size;i++){
      out[0][i]=out[1][i]=0.0f;
    }
    return;
  }

  for (size_t i = 0; i<size; i++){
    if (wav_pos < filemgr.GetSamplesPerChannel()) {
      const int16_t left_sample = GetLeftBufData(wav_pos);
      const int16_t right_sample = GetRightBufData(wav_pos);

      out[0][i] = s162f(left_sample) * 0.5f;
      out[1][i] = s162f(right_sample) * 0.5f;
      wav_pos++;
    }
    else {
      now_playing = false;
      out[0][i]=out[1][i]=0.0f;
    }
  }

  callback_count++;
  // print diagnostic
  if (now_playing && (callback_count - last_report >= pod.AudioSampleRate() / size)) {
    float total_len = filemgr.GetSamplesPerChannel() / pod.AudioSampleRate();
    float elapsed = wav_pos / pod.AudioSampleRate();
    // pod.seed.PrintLine("Playback position: %d / %d", wav_pos, filemgr.GetSamplesPerChannel());
    pod.seed.PrintLine("Playback position: %.2fs / %.2fs", elapsed, total_len);
    last_report = callback_count;
  }
}

int main (void){
  pod.Init();
  pod.seed.StartLog(true);
  pod.SetAudioBlockSize(48);
  pod.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  pod.ProcessAllControls();

  filemgr.SetBuffers(left_buf, right_buf);
  if (!filemgr.Init()){
    pod.seed.PrintLine("filemgr init failed");
    return 1;
  }

  if (!filemgr.ScanWavFiles()){
    pod.seed.PrintLine("reading files failed");
    return 1;
  }
  
  LoadNewFile();
  SetLedColours(Color::PresetColor::GREEN, Color::PresetColor::GREEN);
  System::Delay(3000);

  pod.ClearLeds();
  pod.UpdateLeds();
  pod.StartAdc();
  pod.StartAudio(AudioCallback);

  while(1){
    HandleEncoder();
    HandleButton1();
    HandleButton2();
    HandleKnobs();
    System::Delay(1);
  }
};



