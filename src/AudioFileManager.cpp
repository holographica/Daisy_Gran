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
  /* NOTE: all file names must be less than 128 chars 
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
      TODO: fix how this is handled (or stipulate filenames must be correctly formatted) */
      if (strncmp(fno.fname, "._",2)==0) continue;

      strcpy(name, fno.fname);      
      if (strstr(name, ".wav") || strstr(name, ".WAV")){
        strcpy(names_[count],name);
        // strncpy(names_[count],name, MAX_FNAME_LEN-1);
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
    // stop adding files if we get to 32 
    if (count>=MAX_FILES-1) break;
  }
  f_closedir(&dir);

  // NOTE: converted to 1 index rather than 0 index
  file_count_ = count+1;
  // set curr idx to first file in list
  curr_idx_ = 0;

  for (int i=0; i<MAX_FILES; i++){
    /* stop if filename empty ie no more files */
    /* NOTE: this could poss break sth so double check it works */
    if (names_[i][0]=='\0'){
      break;
    }
    pod_->seed.PrintLine(names_[i]);
  }

  pod_->seed.PrintLine("final count %d files", count);
  // NOTE: here return false if names array is empty
  return true;
}


/* TODO: atm i'm loading by index as this is easier than using the names.
        however i'm still saving names for (if/)when i get a screen ?
*/
bool AudioFileManager::LoadFile(int sel_idx) {
  // NOTE: do i want the index to wrap back around ? 
  // or will i do this within the UI? 
  // if (sel_idx >= file_count_) sel_idx %= file_count_;
  if (sel_idx >= file_count_) return false;

  pod_->seed.Print("loading file: ");
  pod_->seed.PrintLine(names_[sel_idx]);

  if (sel_idx != curr_idx_) {
    pod_->seed.PrintLine("closing file in function");
    pod_->seed.PrintLine("selc: %d  curr: %d",sel_idx, curr_idx_);
    f_close(&curr_file_);
  }

  pod_->seed.PrintLine("ok closed file in function");

  // NOTE: double check this (do i need open existing??)
  if (f_open(&curr_file_, names_[sel_idx], (FA_OPEN_EXISTING | FA_READ)) != FR_OK) return false;

  if (!GetWavHeader(&curr_file_)) return false;

  // NOTE: now checks size + bit depth in wav header function 
  // /* check if file fits into pre-allocated 32mb buffer */
  // if (curr_header_.file_size>=WAV_BUFFER_SIZE){
  //   pod_->seed.PrintLine("selected file exceeds max file size limit. Closing file.");
  //   f_close(&curr_file_);
  //   // NOTE: do i need to empty wav header here? 
  //   return false;
  // }

  // if (curr_header_.bit_depth != BIT_DEPTH){
  //   pod_->seed.PrintLine("File has wrong bit depth. Closing file.");
  //   f_close(&curr_file_);
  //   return false;
  // }






  // TODO: now load file into wav_buffer 
  // NOTE: stipulate (for now) that all files MUST have 16 bit depth for simplicity
  return true;
}

bool AudioFileManager::GetWavHeader(FIL* file){
  WAV_FormatTypeDef header;
  UINT bytes_read;
  FRESULT res = f_read(file, &header, sizeof(WAV_FormatTypeDef), &bytes_read);

  if (res != FR_OK || bytes_read != sizeof(WAV_FormatTypeDef)) return false;

  curr_header_.sample_rate == header.SampleRate;
  curr_header_ .channels == header.NbrChannels;

  /* check if file fits into pre-allocated 32mb buffer */
  if (header.FileSize >= (2*CHNL_BUF_SIZE)) {
    pod_->seed.PrintLine("selected file exceeds max file size limit. Closing file.");
    f_close(file);
    return false;
  }
  else curr_header_.file_size == header.FileSize;

  /* NOTE: checking bit depth here in case it breaks */
  if (header.BitPerSample != BIT_DEPTH) {
    pod_->seed.PrintLine("File has wrong bit depth. Closing file.");
    f_close(file);
    return false;
  }
  else curr_header_.bit_depth == header.BitPerSample;

  /* NOTE: below we find the length of the audio in samples. 
  - we can just do (ChunkSize - 36) - see link below - here ChunkSize === FileSize 
  - convert from bytes to samples (each sample is 16bits ie 2 bytes so divide by 2) 
  - nb: each sample is 16bits because currently we only use 16bit WAV files
  - http://tiny.systems/software/soundProgrammer/WavFormatDocs.pdf
  */
  curr_header_.len_samples == (header.FileSize - 36) / (header.NbrChannels*2);
  pod_->seed.PrintLine("\n\nprinting header data");
  pod_->seed.PrintLine("filesize - 36: %d", (header.FileSize-36));
  pod_->seed.PrintLine("subchunk2size: %d", header.SubCHunk2Size);
  return true;
}



bool AudioFileManager::CloseFile(){
  pod_->seed.Print("closing file: ");
  // curr_idx == index of current file
  pod_->seed.Print(names_[curr_idx_]);
  pod_->seed.PrintLine(" idx: %d "+ curr_idx_);
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
      break;
  }
  for (int x=0;x<20;x++){
      System::Delay(500);
      pod_->led1.Set(255,0,0);
      pod_->UpdateLeds();
      System::Delay(500);
      pod_->led1.Set(255,255,255);
    }
}

// feature:

// first button 

