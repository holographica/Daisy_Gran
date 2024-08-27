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

/* hardware interfaces */
SdmmcHandler sd;
FatFSInterface fsi;
DaisyPod pod;
FIL file;

// ReverbSc reverb;
DSY_SDRAM_BSS ReverbSc reverb;
DSY_SDRAM_BSS SmoothRandomGenerator rng;
/* software classes to run app */
AudioFileManager filemgr(sd, fsi, pod, &file);
static GranularSynth synth(pod, &rng);
GrannyChordApp app(pod, synth, filemgr, reverb);

/* we set rng state here so we can use RNG fns across classes */
uint32_t rng_state;

int main (void){
  pod.Init();
  pod.seed.StartLog(true);
  DebugPrint(pod,"started log");
  // rng.Init(SAMPLE_RATE_FLOAT);
  // rng.SetFreq(1.f);
  // float x;
  // float y;

  // while (1){
  //   pod.ProcessDigitalControls();
  //   if (pod.button1.FallingEdge()){
  //     x = rng.Process();
  //     pod.seed.PrintLine("smooth rng %.9f", x);
  //   }
  // }


  app.Init(left_buf, right_buf);
  app.Run();
}

// let grains through gate when button pressed 
// use button to trigger grain instead of automatically generating it 

// have a rng that chooses which order the notes of the chord are triggered
// could have smal chance of playing random note
// press button to choose random scale

// have prompts for each day 
// 