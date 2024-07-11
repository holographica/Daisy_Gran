#include <stdio.h>
#include "daisy_pod.h"
#include "AudioFileManager.h"
#include "ff.h"

using namespace daisy;

// should i init these here or in main class? 
// here for now, move to main if i need to use them in other classes
// FatFSInterface fsi;
// SdmmcHandler sd;

// init function that takes sd card path, mounts  returns bool for successs?

bool AudioFileManager::Init(){
  SdmmcHandler::Config sd_cfg;
  sd_cfg.Defaults();
  if (sd_->Init(sd_cfg) != SdmmcHandler::Result::OK) {
    BlinkOnSDError('d');
    return false;
  }
  pod_->seed.PrintLine("daisy sd init ok");

  fsi_->Init(FatFSInterface::Config::MEDIA_SD);
  if (f_mount(&fsi_->GetSDFileSystem(),"/",1) != FR_OK){
    BlinkOnSDError('f');
    // TODO: change to enums here + above
    // for sd error blink function
    return false;
  }
  pod_->seed.PrintLine("fatfs init ok");
  
  return true;
}

/* Scan SD card for wavs and save filenames */
bool AudioFileManager::ScanWavFiles(){
  DIR dir;
  FILINFO fno;
  /* NOTE:  all file names must be less than 128 chars 
    TODO: ask if this is fine or if i should actually
          implement length checks + adding null terminator
          (atm check is much easier but not as safe)
  */
  char name[MAX_FNAME_LEN];

  /* clear names array before use */
  memset(names_, '\0', sizeof(names_));

  // do i need this? if just using "/"
  const char* path = fsi_->GetSDPath();

  if (f_opendir(&dir,path) != FR_OK){
    // log error message here
    return false;
  }

  int count=0;
  while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]!=0){
    if (!(fno.fattrib & (AM_HID|AM_DIR))){
      /* NOTE: below, we ignore AppleDouble metadata files starting with '._'
      so if a legit file starts with this prefix, it will be wrongly skipped
      TODO: fix how this is handled (or just stipulate filenames must be correctly formatted) */
      if (strncmp(fno.fname, "._",2)==0) continue;

      strcpy(name, fno.fname);      
      if (strstr(name, ".wav") || strstr(name, ".WAV")){
        strcpy(names_[count],name);
        // strncpy(wavnames_[count],name, MAX_FNAME_LEN-1);
        count++;
      }
    } else {
      // handle directories/hidden files
      if (fno.fattrib & AM_DIR){
        pod_->seed.PrintLine("found dir: %s", fno.fname);
        continue;
      } else if (fno.fattrib & AM_HID){
        pod_->seed.PrintLine("found hidden file: %s", fno.fname);
        continue;
      }
    }
  }
  f_closedir(&dir);

  // NOTE: converted to 1 index rather than 0 index
  file_count_ = count+1;

  for (int i=0; i<MAX_FILES; i++){
    /* stop if filename empty ie no more files */
    /* NOTE: this could poss break sth so double check it works */
    if (names_[i][0]=='\0'){
      break;
    }
    pod_->seed.PrintLine(names_[i]);
  }

  pod_->seed.PrintLine("final count %d files", count);
  return true;
}


/* TODO: atm i'm loading by index as this is easier than using the names.
        however i'm still saving names for (if/)when i get a screen ?
*/
bool AudioFileManager::LoadFile(int sel_idx) {
  if (sel_idx >= file_count_) return false;

  pod_->seed.Print("loading file: ");
  pod_->seed.PrintLine(names_[sel_idx]);

  if (sel_idx != curr_idx_) {
    f_close(&curr_file_);
  }
  
  // NOTE: double check this
  return f_open(&curr_file_, names_[sel_idx], (FA_OPEN_EXISTING | FA_READ)) == FR_OK;
}

bool AudioFileManager::CloseFile(){
  return f_close(&curr_file_) == FR_OK;
}

// getters for wav file info

// getter for pointer to audio buffer that file is loaded into
// getter for buffer size?

// SD card init error! 
void AudioFileManager::BlinkOnSDError (char type){
  switch(type) {
    case 'd': 
      pod_->seed.PrintLine("Daisy SD init error");
      break;
    case 'f':
      pod_->seed.PrintLine("FatFS SD error");
  }
  for (int x=0;x<20;x++){
      System::Delay(500);
      pod_->led1.Set(255,0,0);
      pod_->UpdateLeds();
      System::Delay(500);
      pod_->led1.Set(255,255,255);
    }
}