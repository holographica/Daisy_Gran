#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "ff.h"
#include "AudioFileManager.h"

using namespace daisy;

// allocate int16s - each is 2bytes -> 16mb
constexpr size_t CHANNEL_BUF_SIZE = 8*1024*1024;
// constexpr size_t OUTPUT_BUFFER_SIZE = 48000*10 // 10seconds @ 48kHz

SdmmcHandler sd;
FatFSInterface fsi;
DaisyPod pod;
FIL file;
AudioFileManager filemgr(&sd, &fsi, &pod, &file);

DSY_SDRAM_BSS alignas(16) int16_t left_buf[CHANNEL_BUF_SIZE];
DSY_SDRAM_BSS alignas(16) int16_t right_buf[CHANNEL_BUF_SIZE];

size_t wav_pos = 0;
bool change_file = false;
bool now_playing = false;
int curr_file_idx = 0;
uint32_t numsamples = 0;

/* TODO here:
- set up error message function so i can turn debug mode off/on with a bool
- then change all pod prints to error msg so only prints if in debug mode
*/

int GetLeftBufData(size_t pos){
  return (pos < CHANNEL_BUF_SIZE) ? left_buf[pos] : 0;
}

int GetRightBufData(size_t pos){
  return (pos < CHANNEL_BUF_SIZE) ? right_buf[pos] : 0;
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

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  static uint32_t callback_count = 0;
  static uint32_t last_report = 0;

  if (wav_pos >= CHANNEL_BUF_SIZE) {
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
      int16_t left_sample = GetLeftBufData(wav_pos);
      int16_t right_sample = GetRightBufData(wav_pos);

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
      pod.seed.PrintLine("Playback position: %d / %d", wav_pos, filemgr.GetSamplesPerChannel());
      last_report = callback_count;
  }
}

int main (void){
  pod.Init();
  pod.seed.StartLog(true);
  pod.SetAudioBlockSize(48);
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

  pod.StartAdc();
  pod.StartAudio(AudioCallback);
  while(1){
    HandleEncoder();
    HandleButton1();
    HandleButton2();
    System::Delay(1);
  }
};

