#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "ff.h"
#include "AudioFileManager.h"

using namespace daisy;

// allocate floats - each is 4bytes -> 16mb
constexpr size_t CHANNEL_BUF_SIZE = 4*1024*1024;
// constexpr size_t OUTPUT_BUFFER_SIZE = 48000*10 // 10seconds @ 48kHz

SdmmcHandler sd;
FatFSInterface fsi;
DaisyPod pod;
FIL file;
AudioFileManager filemgr(&sd, &fsi, &pod, &file);

DSY_SDRAM_BSS float left_buf[CHANNEL_BUF_SIZE];
DSY_SDRAM_BSS float right_buf[CHANNEL_BUF_SIZE];

uint32_t wav_pos = 0;
bool change_file = false;
bool now_playing = false;
int curr_file_idx =0;

// NB: CAN USE daisy::AudioHandle::GetConfig() to get the global config for audio
// (from daisy::AudioHandle class ref on libDaisy docs)
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
  if (!now_playing){
    for (size_t i=0; i<size;i++){
      out[0][i]=out[1][i]=0.0f;
    }
    return;
  }

  for (size_t i = 0; i<size; i++){
    if (wav_pos < filemgr.GetNumSamples()) {
      out[0][i] = filemgr.GetLeftBuffer()[wav_pos]*0.5f;
      out[1][i] = filemgr.GetRightBuffer()[wav_pos]*0.5f;
      wav_pos++;
    }
    else {
      out[0][i]=out[1][i]=0.0f;
    }
  }
  return;
}

void HandleEncoder(){
  pod.encoder.Debounce();
  int32_t inc = pod.encoder.Increment();
  char fname[128];

  if (inc!=0){
    curr_file_idx+=inc;
    if (curr_file_idx<0) { curr_file_idx = filemgr.GetFileCount() - 1; }
    if (curr_file_idx >= filemgr.GetFileCount()) { curr_file_idx = 0; }
    filemgr.GetName(curr_file_idx, fname);
    pod.seed.PrintLine("selected new file idx %d %s", curr_file_idx, fname);
  }
  // if (pod.encoder.RisingEdge()){
  //   change_file = true;
  //   now_playing = false;
  //   filemgr.GetName(curr_file_idx, fname);
  //   pod.seed.PrintLine("loading file idx %d %s", curr_file_idx, fname);
  // }
}


void HandleButton1(){
  char fname[128];
  pod.button1.Debounce();
  if (pod.button1.RisingEdge()){
      change_file = true;
      now_playing = false;
      filemgr.GetName(curr_file_idx, fname);
      pod.seed.PrintLine("loading file idx %d %s", curr_file_idx, fname);
    }
}


int main (void){
  pod.Init();
  pod.seed.StartLog(true);
  pod.SetAudioBlockSize(4);
  pod.ProcessAllControls();

  filemgr.SetBuffers(left_buf, right_buf, CHANNEL_BUF_SIZE);
  if (!filemgr.Init()){
    pod.seed.PrintLine("filemgr init failed");
    return 1;
  }

  if (!filemgr.ScanWavFiles()){
    pod.seed.PrintLine("reading files failed");
    return 1;
  }

  if (!filemgr.LoadFile(curr_file_idx)){
    pod.seed.PrintLine("loading file 0 failed");
    return 1;
  }
  now_playing = true;
  pod.seed.PrintLine("now playing track %d",curr_file_idx);

  pod.StartAdc();
  pod.StartAudio(AudioCallback);
  // pod.seed.PrintLine("started audio callback.");

  while(1){
    HandleEncoder();
    HandleButton1();
    if (change_file){
      filemgr.LoadFile(curr_file_idx);
      wav_pos=0;
      change_file = false;
      now_playing = true;
      pod.seed.PrintLine("now playing track %d",curr_file_idx);
    }
    System::Delay(1);
  }
};
