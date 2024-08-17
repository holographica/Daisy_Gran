#pragma once

enum class AppState{
  SelectFile,
  PlayWAV,
  Synthesis,
  RecordIn,
  // ChordMode
  // RecordOut? would coexist with another state so maybe not? 
  Error
};

enum class SynthMode{
  /* knob1 controls param 1, knob2 controls param 2 */
  Size_Position = 0,        /* param 1 = grain size, param 2 = grain spawn position */
  /* knob 1 controls randomness applied to param 1, knob 2 controls randomness for param 2 */
  Size_Position_Rnd,
  Pitch_ActiveGrains,   /* param 1 = grain pitch, param 2 = num of active grains */
  Pitch_ActiveGrains_Rnd,
  PhasorMode_EnvType,   /* param 1 = phasor mode, param 2 = grain envelope type */
  PhasorMode_EnvType_Rnd,
  /* knob1 controls pan, knob2 controls randomness of pan */
  Pan_PanRnd,
  Reverb,
  Filter
};