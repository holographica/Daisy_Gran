#pragma once
#include "daisy_pod.h"

#ifdef DEBUG_MODE
#define DebugPrint(pod, ...) do { (pod).seed.PrintLine(__VA_ARGS__); } while (0)
#else
#define DebugPrint(pod, ...) do {} while (0)
#endif
