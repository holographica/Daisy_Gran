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

using namespace daisy;
using namespace daisysp;
using namespace std;

/* SDRAM buffers for storing WAV files or recorded input audio */
DSY_SDRAM_BSS alignas(16) int16_t left_buf[CHNL_BUF_SIZE_SAMPS];
DSY_SDRAM_BSS alignas(16) int16_t right_buf[CHNL_BUF_SIZE_SAMPS];

DSY_SDRAM_BSS ReverbSc reverb; 

/* SDRAM buffer for temporarily storing recorded output audio
    before it's written to SD card */
// DSY_SDRAM_BSS alignas(16) int16_t record_out_buf[RECORD_OUT_BUF_SIZE_SAMPS];

/* hardware interfaces */
DSY_SDRAM_BSS SdmmcHandler sd;
// __attribute__((section(".axi_sram")))SdmmcHandler sd;
// SdmmcHandler sd;
DSY_SDRAM_BSS FatFSInterface fsi;
// __attribute__((section(".axi_sram")))FatFSInterface fsi;
DaisyPod pod;
DSY_SDRAM_BSS FIL file;

/* software classes to run app */
//NOTE: can't put this on SDRAM
// CAN put it on axi_sdram
__attribute__((section(".axi_sdram")))AudioFileManager filemgr(sd, fsi, pod, &file);
// AudioFileManager filemgr(sd, fsi, pod, &file);
// DSY_SDRAM_BSS AudioFileManager filemgr(sd, fsi, pod, &file);
__attribute__((section(".axi_sram")))static GranularSynth synth(pod);

// DSY_SDRAM_BSS static GranularSynth synth(pod);
// __attribute__((section(".axi_sram")))GrannyChordApp app(pod, synth, filemgr);
// __attribute((section(".sram")))GrannyChordApp app(pod, synth, filemgr);
// DSY_SDRAM_BSS GrannyChordApp app(pod, synth, filemgr);
GrannyChordApp app(pod, synth, filemgr);

/* we set rng state here so we can use RNG fns across classes */
uint32_t rng_state;

int main (void){
  pod.Init();
  pod.seed.StartLog(true);
  DebugPrint(pod,"started log");
  pod.led1.SetBlue(1);
  pod.UpdateLeds();
  SdmmcHandler::Config cfg;
  cfg.Defaults();
  // if (sd.Init(cfg)!=SdmmcHandler::Result::OK){
  //   pod.seed.PrintLine("sd init failed");
  // } else {
  //   pod.seed.PrintLine("sd init ok");
  // }
  // fsi.Init(FatFSInterface::Config::MEDIA_SD);
  // if (f_mount(&fsi.GetSDFileSystem(),"/",1)!=FR_OK){
  //   pod.seed.PrintLine("fsi init failed");
  // }
  // else {
  //   pod.seed.PrintLine("fsi init ok");
  // }


  app.Init(left_buf, right_buf,&reverb);
  // app.Run();
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