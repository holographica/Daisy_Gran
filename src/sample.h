#pragma once

struct Sample{
  float left;
  float right;
  Sample& operator+=(const Sample& new_sample){
    left += new_sample.left;
    right += new_sample.right;
    return *this;
  }
};
