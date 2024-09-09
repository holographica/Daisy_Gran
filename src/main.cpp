#ifdef DEBUG_MODE
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
#include "DaisySP-LGPL-FX/reverb.h"
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

DSY_SDRAM_BSS ReverbSc reverb;
/* software classes to run app */
AudioFileManager filemgr(sd, fsi, pod, &file);
static GranularSynth synth(pod);
GrannyChordApp app(pod, synth, filemgr, reverb);

/* we set rng state here so we can use RNG fns across classes */
uint32_t rng_state;

int main (void){
  pod.Init();
  #ifdef DEBUG_MODE
  pod.seed.StartLog(true);
  #endif

  app.Init(left_buf, right_buf);
  app.Run();
}