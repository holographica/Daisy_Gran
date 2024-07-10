// TODO: make a .h file once this class is somehwat done

#include <stdio.h>
#include "daisy_pod.h"
// #include "daisysp.h"
#include "AudioFileManager.h"
#include "ff.h"

using namespace daisy;

FatFSInterface fsi;
SdmmcHandler sd;
// functions i need

// init function that takes sd card path, mounts  returns bool for successs?

bool AudioFileManager::Init(){
  SdmmcHandler::Config sd_cfg;
  sd_cfg.Defaults();
  if (sd.Init(sd_cfg) != SdmmcHandler::Result::OK) {
    return false;
  }

  fsi.Init(FatFSInterface::Config::MEDIA_SD);
  if (f_mount(&fsi.GetSDFileSystem(),"/",1) != FR_OK){
    // TODO: return enums here + above
    // for sd error blink function to print to log? 
    return false;
  }

  return true;
}

bool AudioFileManager::ScanWavFiles(){
  // TODO: change to char arary
  // std::vector<std::string> wav_files;
  DIR dir;
  FILINFO fno;
  char name[256];
  int count=0;
  // do i need this? if just using "/"
  const char* path = fsi.GetSDPath();

  if (f_opendir(&dir,path) != FR_OK){
    
  }



}


// scan wav files fn from main file - returns list of filenames

// load a file from sd card by filename/pointer to name - return bool

// close file 

// getters for wav file info

// getter for pointer to audio buffer that file is loaded into
// getter for buffer size?

