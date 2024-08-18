#ifdef DEBUG_MODE // TODO: remove
#pragma message("Debug mode is ON")
#else
#pragma message("Debug mode is OFF")
#endif

#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "AudioFileManager.h"
#include "GranularSynth.h"
#include "GrannyChordApp.h"
#include "constants_utils.h"
#include "debug_print.h"
#include "zita-rev1/source/reverb.h"

//NOTE: FOUND SAMPLE RATE CONVERTER - SEE IF IT WORKS WELL
// it's at stmlib/sample_rate_converter.h 
// in daisyexamples - will have to copy stmlib
CpuLoadMeter cpumeter;

using namespace daisy;
using namespace daisysp;
using namespace std;

/* SDRAM buffers for storing WAV files or recorded input audio */
DSY_SDRAM_BSS alignas(16) int16_t left_buf[CHNL_BUF_SIZE_SAMPS];
DSY_SDRAM_BSS alignas(16) int16_t right_buf[CHNL_BUF_SIZE_SAMPS];

/* hardware interfaces */
DATA_ DaisyPod pod;
DATA_ SdmmcHandler sd;
DATA_ FatFSInterface fsi;
DATA_ FIL file;
/* software classes to run app */
// AudioFileManager filemgr(sd, fsi, pod, &file);
DATA_ static AudioFileManager filemgr;

// DSY_SDRAM_BSS AudioFileManager filemgr(sd, fsi, pod, &file);
// static DTCMRAM_BSS GranularSynth synth(pod);
DATA_ static GranularSynth synth;

// static DTCMRAM_BSS GrannyChordApp app(pod, synth, filemgr);
DATA_ static GrannyChordApp app;

/* we set rng state here so we can use RNG fns across classes */
uint32_t rng_state;

bool InitObjects(){
  SdmmcHandler::Config sd_cfg;
  sd_cfg.Defaults();
  if (sd.Init(sd_cfg) != SdmmcHandler::Result::OK) {
    return false;
  }
  fsi.Init(FatFSInterface::Config::MEDIA_SD);
  if (f_mount(&fsi.GetSDFileSystem(),"/",1) != FR_OK){
    return false;
  }
  filemgr.Init(sd, fsi, pod, nullptr);
  synth.Init(pod, left_buf, right_buf, 0);
  app.Init(pod,synth, filemgr, left_buf, right_buf);
  return true;
}


int main (void){
  pod.Init();
  #ifdef DEBUG_MODE
  pod.seed.StartLog(true);
  DebugPrint(pod,"started log");
  #endif
  if (!InitObjects()){
    pod.led1.SetRed(1);
    pod.led2.SetRed(1);
    // DebugPrint("")
    return 1;
  }
  pod.led1.SetBlue(1);
  pod.UpdateLeds();
  cpumeter.Init(pod.AudioSampleRate(), pod.AudioBlockSize());
  app.PassCPUMeter(cpumeter);

  /* call onblockstart at start of audiocb, onblockend at end
  then print:
   // get the current load (smoothed value and peak values)
        const float avgLoad = cpuLoadMeter.GetAvgCpuLoad();
        const float maxLoad = cpuLoadMeter.GetMaxCpuLoad();
        const float minLoad = cpuLoadMeter.GetMinCpuLoad();
        // print it to the serial connection (as percentages)
        hw.PrintLine("Processing Load %:");
        hw.PrintLine("Max: " FLT_FMT3, FLT_VAR3(maxLoad * 100.0f));
        hw.PrintLine("Avg: " FLT_FMT3, FLT_VAR3(avgLoad * 100.0f));
        hw.PrintLine("Min: " FLT_FMT3, FLT_VAR3(minLoad * 100.0f));
        // don't spam the serial connection too much
        System::Delay(500);
  */


  // DebugPrint(pod, "app init done.");
  // filemgr.LoadFile(6);
  // DebugPrint(pod, "finished loading file.");

  // pod.StartAdc();
  // pod.StartAudio(AudioCallback);

  app.Run();
  // while(true){

  // }

}

// let grains through gate when button pressed 
// use button to trigger grain instead of automatically generating it 

// have a rng that chooses which order the notes of the chord are triggered
// could have smal chance of playing random note
// press button to choose random scale

// have prompts for each day 
// 