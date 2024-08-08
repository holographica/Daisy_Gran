#pragma once

enum class AppState {
  Startup,
  SelectFile,
  PlayWAV,
  Synthesis
};

enum class SynthMode {
  SizePosition,
  PitchGrains
  // add more eg spray/pan, randomness, phasormode/envelope etc
};