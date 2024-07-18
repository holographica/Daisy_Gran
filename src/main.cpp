#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "ff.h"
#include "AudioFileManager.h"
// #include "daisy_seed.h"
// #include "daisy_core.h"

using namespace daisy;

// NOTE: changed to separate L/R channel buffers
// constexpr int WAV_BUFFER_SIZE = 32*1024*1024; // 32MB buffer for loading wav files
constexpr size_t CHANNEL_BUF_SIZE = 16*1024*1024;


DaisyPod pod;
FatFSInterface fsi;
SdmmcHandler sd;
FIL file;

// float DSY_SDRAM_BSS wav_buffer[WAV_BUFFER_SIZE];
float DSY_SDRAM_BSS left_buf[CHANNEL_BUF_SIZE];
float DSY_SDRAM_BSS right_buf[CHANNEL_BUF_SIZE];

// NOTE: do we need constexpr or just const?
// constexpr size_t OUTPUT_BUFFER_SIZE = 48000*10 // 10seconds @ 48kHz

// void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
// {
//   // NB: CAN USE daisy::AudioHandle::GetConfig() to get the global config for audio
//           // (from daisy::AudioHandle class ref on libDaisy docs)

//   // do something here ie loop over size, process samples, send them to output

// }

// functions: audiocallback, main, error handling functions? 

int main (void)
{
  pod.Init();
  pod.seed.StartLog(true);
  pod.seed.PrintLine("log is working");

  // should i define /declare this at top? 
  pod.SetAudioBlockSize(4);

  // check block size/SR are as expected
  pod.seed.PrintLine("block size %d", int(pod.AudioBlockSize()));
  pod.seed.PrintLine("sr %d", int(pod.AudioSampleRate()));

  pod.led1.Set(255,255,255);
  pod.UpdateLeds();

  // pass pointers for constructor (don't need ptr for buffer - array name is pointer itself)
  AudioFileManager filemgr(&sd, &fsi, &pod);
  filemgr.SetBuffers(left_buf, right_buf, CHANNEL_BUF_SIZE)
  if (!filemgr.Init()){
    pod.seed.PrintLine("filemgr init failed");
  }
  pod.seed.PrintLine("filemgr init success");

  const char* path = fsi.GetSDPath();
  pod.seed.Print("from main - sd path is:");
  pod.seed.Print("(%s)\n",path);

  pod.ClearLeds();

  // TODO: decide how to pass 
  // filenames/idxs to UI if using screen
  if (filemgr.ScanWavFiles()){
    pod.seed.PrintLine("success reading files\n");
    pod.led1.SetGreen(1);
  } else {
    pod.seed.PrintLine("reading files failed");
    pod.led1.SetRed(1);
  }
  pod.UpdateLeds();

  if (filemgr.LoadFile(3)){
    pod.seed.PrintLine("success loading file 3");
  } else {
    pod.seed.PrintLine("loading file 3 failed");
  }
  pod.seed.PrintLine("--------\n");
  if (filemgr.CloseFile()){
    pod.seed.PrintLine("success closing file 0");
  } else {
    pod.seed.PrintLine("closing file 0 failed");
  }
  pod.seed.PrintLine("first file load/close done\n");

  if (filemgr.LoadFile(1)){
    pod.seed.PrintLine("success loading file 1");
  } else {
    pod.seed.PrintLine("loading file 1 failed");
  }
  pod.seed.PrintLine("second file load done\n");

  if (filemgr.LoadFile(2)){
    pod.seed.PrintLine("success loading file 2");
  } else {
    pod.seed.PrintLine("loading file 2 failed");
  }
  if (filemgr.CloseFile()){
    pod.seed.PrintLine("success closing file 2");
  } else {
    pod.seed.PrintLine("closing file 2 failed");
  }
  pod.seed.PrintLine("third file load/close done");






  // now pass to granular instrument or file mgr 
}
