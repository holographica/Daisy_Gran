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

// NB: CAN USE daisy::AudioHandle::GetConfig() to get the global config for audio
// (from daisy::AudioHandle class ref on libDaisy docs)
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
  for (size_t i = 0; i<size; i++){
    if (wav_pos < filemgr.GetNumSamples()) {
      out[0][i] = filemgr.GetLeftBuffer()[wav_pos]*0.5f;
      out[1][i] = filemgr.GetRightBuffer()[wav_pos]*0.5f;
      wav_pos++;
    }
    else {
      out[0][i]=0.0f;
      out[1][i]=0.0f;
    }
  }
}


int main (void)
{
  pod.Init();
  pod.seed.StartLog(true);
  pod.SetAudioBlockSize(4);

  filemgr.SetBuffers(left_buf, right_buf, CHANNEL_BUF_SIZE);
  if (!filemgr.Init()){
    pod.seed.PrintLine("filemgr init failed");
  }

  pod.seed.PrintLine("init passed.\n");
  // // TODO: decide how to pass 
  // // filenames/idxs to UI if using screen
  if (filemgr.ScanWavFiles()){
    pod.seed.PrintLine("success scanning files\n");
  } else {
    pod.seed.PrintLine("reading files failed");
    pod.led1.SetRed(1);
    pod.UpdateLeds();
  }

  if (filemgr.LoadFile(5)){
    pod.seed.PrintLine("success loading file 0");
  } else {
    pod.seed.PrintLine("loading file 0 failed");
  }
  
  pod.seed.PrintLine("now playing track 0");
  pod.seed.PrintLine("callback rate: %.3f", pod.AudioCallbackRate());
  for (int i =0; i<3000; i++) {
    pod.seed.DelayMs(1);
    if (i%1000==0) { pod.seed.PrintLine("%i seconds",(i/1000)); }
  }

  pod.StartAdc();
  pod.StartAudio(AudioCallback);
  pod.seed.PrintLine("got to final bit");

} 

  // pod.seed.PrintLine("--------\n");
  // if (filemgr.CloseFile()){
  //   pod.seed.PrintLine("success closing file 0");
  // } else {
  //   pod.seed.PrintLine("closing file 0 failed");
  // }
  // pod.seed.PrintLine("first file load/close done\n");

  // if (filemgr.LoadFile(1)){
  //   pod.seed.PrintLine("success loading file 1");
  // } else {
  //   pod.seed.PrintLine("loading file 1 failed");
  // }
  // pod.seed.PrintLine("second file load done\n");

  // if (filemgr.LoadFile(2)){
  //   pod.seed.PrintLine("success loading file 2");
  // } else {
  //   pod.seed.PrintLine("loading file 2 failed");
  // }
  // if (filemgr.CloseFile()){
  //   pod.seed.PrintLine("success closing file 2");
  // } else {
  //   pod.seed.PrintLine("closing file 2 failed");
  // }
  // pod.seed.PrintLine("third file load/close done");

  // pod.led1.SetGreen(1);
  // pod.UpdateLeds();





  // now pass to granular instrument or file mgr 
