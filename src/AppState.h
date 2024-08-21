#pragma once

enum class AppState{
  SelectFile,
  PlayWAV,
  Synthesis,
  RecordIn,
  ChordMode,
  Error
};

enum class SynthMode{
  /* knob1 controls param 1, knob2 controls param 2 */
  Size_Position,        /* param 1 = grain size, param 2 = grain spawn position */
  /* knob 1 controls randomness applied to param 1, knob 2 controls randomness for param 2 */
  Size_Position_Rnd,
  Pitch_ActiveGrains,   /* param 1 = grain pitch, param 2 = num of active grains */
  Pitch_ActiveGrains_Rnd,
  /* NOTE: pan is diff to other modes */
  Pan_Direction, /* knob1 controls pan, knob2 controls phasor direction */
  PanRnd_Chorus,
  /* FX modes */
  Reverb,
  Filter
};
