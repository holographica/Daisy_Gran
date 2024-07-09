#include <stdio.h>
#include "daisy_pod.h"
#include "AudioFileManager.h"

using namespace daisy;

DaisyPod pod;
AudioFileManager filemgr; 

/*

params: 
- vector of grains 
- float array for audio buffer (> vector? consider performance vs ease)
- buffer size
- synth params eg density, spray, grain size etc
- last grain time? 


functions:
- set audio buffer
- process grains
- set [param] eg (global - all grains) density, grain size, pitch, spray etc
- update grains? (private - shouldn't be accessible outside this class)
  - handles triggering new grains, resetting them after they finish, managing grain lifetimes   

- function to check if we should trigger new grain? incl in update grains? 
- ie triggers grains regularly based on density (add randomness)


*/