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
  Pitch_ActiveGrains,   /* param 1 = grain pitch, param 2 = num of active grains */
  Pan_Direction, /* knob1 controls pan, knob2 controls phasor direction */
  RndAmount_RndFreq,
  /* FX modes */
  StereoRotate,
  Chorus,
  Reverb,
  Filter
};
