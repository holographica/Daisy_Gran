#pragma once

const int SAMPLE_RATE = 48000;
const float SAMPLE_RATE_FLOAT = 48000.f;
const size_t ABS_CHNL_BUF_SIZE = 16*1024*1024;
constexpr size_t CHNL_BUF_SIZE_SAMPS = 8*1024*1024;
const int BIT_DEPTH = 16;

/* converts milliseconds to number of samples */
inline constexpr float MsToSamples(float ms){
  return SAMPLE_RATE_FLOAT * (ms*0.001f);
}

/* converts number of samples to milliseconds */
inline constexpr float SamplesToMs(float samples){
  return (samples * 1000.0f) / SAMPLE_RATE_FLOAT; 
}

/* using quake inverse root from 
https://www.geeksforgeeks.org/fast-inverse-square-root/
this works because x*(1/sqrt(x)) === x  */
inline constexpr float Fast_Sqrt(float num){ 
  const float threehalfs = 1.5F; 
  float x2 = num * 0.5F; 
  float y = num; 
  long i = * ( long * ) &y; 
  i = 0x5f3759df - ( i >> 1 ); 
  y = * ( float * ) &i; 
  y = y * ( threehalfs - ( x2 * y * y ) ); 
// now multiply by original num to get the regular sqrt
  y *= num;
  return y; 
} 

