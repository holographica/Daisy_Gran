#include <stdio.h>
#include "daisysp.h"
#include "daisy_pod.h"
#include "ff.h"
// #include "daisy_seed.h"
// #include "daisy_core.h"

using namespace daisy;

DaisyPod pod;
FatFSInterface fsi;
SdmmcHandler sd;
FIL file;

void blinkOnSDError(char type);
bool scanWavFiles(const char* path);

// void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
// {
//   // NB: CAN USE daisy::AudioHandle::GetConfig() to get the global config for audio
//           // (from daisy::AudioHandle class ref on libDaisy docs)

//   // do something here ie loop over size, process samples, send them to output
//   // blinkOnSDError();
// }

// functions: audiocallback, main, error handling functions? 

int main (void)
{
  // init Daisy Pod
  pod.Init();
  pod.seed.StartLog(true);
  pod.seed.PrintLine("log is working");

  size_t blocksize = 4;
  pod.SetAudioBlockSize(blocksize);

  // check block size/SR are as expected
  pod.seed.PrintLine("block size %d", int(pod.AudioBlockSize()));
  pod.seed.PrintLine("sr %d", int(pod.AudioSampleRate()));

  pod.led1.Set(255,255,255);
  pod.UpdateLeds();

  // init SD card
  SdmmcHandler::Config sd_cfg;
  sd_cfg.Defaults();
  sd.Init(sd_cfg);

  // handle SD card init error
  if (sd.Init(sd_cfg) != SdmmcHandler::Result::OK) {
    blinkOnSDError('d');
    return 1;
  }

  pod.seed.PrintLine("daisy sd init ok");
  pod.DelayMs(500);
  pod.ClearLeds();

  // // init fsi, mount filesystem
  fsi.Init(FatFSInterface::Config::MEDIA_SD);
  if (f_mount(&fsi.GetSDFileSystem(),"/",1) != FR_OK){
    blinkOnSDError('i');
    return 1;
  }
  pod.seed.PrintLine("fatfs init ok");

  const char* path = fsi.GetSDPath();
  pod.seed.Print("sd path is:");
  pod.seed.Print("(%s)\n",path);

// NOW PASS TO FILE MGR TO DO OTHER SD CARD OPS, LOAD FILES ETC
// POSSIBLY TAKE ABOVE SD OPS OUT OF THIS FILE TOO ? WILL HAVE TO SEE - MAY HAVE TO STAY HERE


  if (scanWavFiles(path)){
    pod.seed.PrintLine("success reading filenames");
    pod.led1.SetGreen(1);
  } else {
    pod.seed.PrintLine("reading filenames failed");
    pod.led1.SetRed(1);
  }
  pod.UpdateLeds();

  // now pass to granular instrument or file mgr 
}



bool scanWavFiles(const char* path) {
  /* TODO: change vector list to char* array to optimise */
  std::vector<std::string> files;
  DIR dir;
  FILINFO fno;
  char name[256];
  int count=0;
  // size_t curr_index;
  // WavFileInfo info[16];

  /* TODO: return res code here for more error info 
  FRESULT res = FR_OK;
  res = f_opendir(&dir, path);
  if (res != FR_OK) { */
  if (f_opendir(&dir,path)!=FR_OK){
    // pod.seed.PrintLine("failed to open dir: error %d",res);
    pod.seed.PrintLine("failed to open dir");
    return false;
  }

  while (f_readdir(&dir, &fno)==FR_OK && fno.fname[0]!=0){
    if (!(fno.fattrib & (AM_HID | AM_DIR))){
      // name = fno.fname;
      strcpy(name, fno.fname);

      /* NOTE: below, we ignore AppleDouble metadata files starting with '._'
      so if a legit file starts with this prefix, it will be wrongly skipped
      TODO: fix how this is handled (or just stipulate filenames must be correctly formatted) */
      if (strncmp(name, "._",2)==0) continue;
      
      std::string fileName(name);
      if (strstr(name, ".wav") || strstr(name, ".WAV")){
      // if (fileName.substr(fileName.find_last_of(".") + 1) == "wav") {
        count++;
        files.push_back(fileName);
      }
    } else {
      // handle directories/hidden files
      if (fno.fattrib & AM_DIR){
        pod.seed.PrintLine("found dir: %s", fno.fname);
        continue;
      } else if (fno.fattrib & AM_HID){
        pod.seed.PrintLine("found hidden file: %s", fno.fname);
        continue;
      }
    }
  }
  f_closedir(&dir);
  for (const std::string& file: files){
    pod.seed.PrintLine(file.c_str());
  }
  pod.seed.PrintLine("final count is %d files", count);
  return true;
}





// SD card init error! 
void blinkOnSDError (char type)
{
  switch(type) {
    case 'd': 
      pod.seed.PrintLine("Daisy SD init error");
      break;
    case 'f':
      pod.seed.PrintLine("FatFS SD error");
  }
  for (int x=0;x<20;x++){
      System::Delay(500);
      pod.led1.Set(255,0,0);
      pod.UpdateLeds();
      System::Delay(500);
      pod.led1.Set(0,0,0);
    }
}
