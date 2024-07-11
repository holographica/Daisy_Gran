#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "ff.h"
#include "AudioFileManager.h"
// #include "daisy_seed.h"
// #include "daisy_core.h"

using namespace daisy;

DaisyPod pod;
FatFSInterface fsi;
SdmmcHandler sd;
FIL file;

// NOTE: do we need constexpr or just const? int or size_t?
// constexpr int FILE_BUFFER_SIZE = 32*1024*1024; // 32MB buffer for loading wav files
// constexpr int OUTPUT_BUFFER_SIZE = 48000*10 // 10seconds @ 48kHz
// float DSY_SDRAM_BSS wav_buffer[MAX_FILE_BUFFER_SIZE]

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

  // pass pointers for constructor
  AudioFileManager filemgr(&sd, &fsi, &pod);
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
    pod.seed.PrintLine("success reading filenames");
    pod.led1.SetGreen(1);
  } else {
    pod.seed.PrintLine("reading filenames failed");
    pod.led1.SetRed(1);
  }
  pod.UpdateLeds();

  // now pass to granular instrument or file mgr 
}
