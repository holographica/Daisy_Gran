#pragma once
// #include <cstdarg>
#include "daisy_pod.h"

#ifdef DEBUG_MODE
void DebugPrint(const char* format, ...){
  va_list va;
  va_start(va, format);
  pod.seed.PrintLine(format, va);
  va_end(va);
}
#endif


