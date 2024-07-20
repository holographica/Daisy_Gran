#include <stdio.h>
#include "AudioFileManager.h"
#include <vector>

using namespace daisy;

bool AudioFileManager::Init(){
  SdmmcHandler::Config sd_cfg;
  sd_cfg.Defaults();
  // sd_cfg.speed = SdmmcHandler::Speed::SLOW; // Uncomment if available
  // sd_cfg.width = SdmmcHandler::BusWidth::BITS_1;
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
  char name[MAX_FNAME_LEN];
  memset(names_, '\0', sizeof(names_));

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
        count++;
      }
    } else {
      /* handle directories/hidden files */
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
  curr_idx_ = 0;

  for (int i=0; i<MAX_FILES; i++){
    /* stop if filename empty ie no more files */
    /* NOTE: this could poss break sth */
    if (names_[i][0]=='\0'){
      break;
    }
    pod_->seed.PrintLine(names_[i]);
  }

  pod_->seed.PrintLine("final count %d files", count);
  // TODO: return false if names array is empty
  return true;
}

/* TODO: atm i'm loading by index as this is easier than using the names.
        however i'm still saving names for (if/)when i get a screen ?
*/
bool AudioFileManager::LoadFile(int sel_idx) {
  uint32_t start_time = System::GetNow();

  // NOTE: do i want the index to wrap back around ? 
  // or will i do this within the UI? 
  // if (sel_idx >= file_count_) sel_idx %= file_count_;
  if (sel_idx >= file_count_) return false;

  pod_->seed.Print("loading file: ");
  pod_->seed.PrintLine(names_[sel_idx]);

  if (sel_idx != curr_idx_) {
    pod_->seed.PrintLine("closing file in function");
    pod_->seed.PrintLine("selc: %d  curr: %d",sel_idx, curr_idx_);
    f_close(curr_file_);
  }

  FRESULT open_res = f_open(curr_file_, names_[sel_idx], (FA_OPEN_EXISTING | FA_READ));
  if (open_res != FR_OK) {
    pod_->seed.PrintLine("FAILED TO F_OPEN: %d", open_res);
    return false;
  }
  pod_->seed.PrintLine("open file success: %s \n", names_[sel_idx]);

  if (!GetWavHeader(curr_file_)){
    f_close(curr_file_);
    pod_->seed.PrintLine("Closed file.");
    return false;
  }

  if (!LoadAudioData()) {
    pod_->seed.PrintLine("failed to load audio data");
    return false;
  }
  uint32_t end_time = System::GetNow();
  float load_time = (end_time-start_time)/1000.0f;
  pod_->seed.PrintLine("loaded in %.6f seconds", load_time);

  return true;
}


// bool AudioFileManager::GetWavHeader(FIL *file){
  WAV_FormatTypeDef header;
//   UINT bytes_read;
//   // f_lseek(file,0); 
//   FRESULT res = f_read(file, &header, sizeof(WAV_FormatTypeDef), &bytes_read);

//   if (res != FR_OK || bytes_read != sizeof(WAV_FormatTypeDef)) {
//     pod_->seed.PrintLine("Failed to read WAV header. Error code: %d, Bytes read: %d", res, bytes_read);
//     return false;
//   }
//   // TODO: check if 48khz, if not then resample !!
//   // TODO: create resampling function, copy new data to channel bufs, update params below
//   curr_header_.sample_rate = (int)header.SampleRate;
//   curr_header_ .channels = (short)header.NbrChannels;

//   /* check if audio data fits into pre-allocated 32mb buffer */
//   if (header.SubCHunk2Size >= (2*ABS_CHNL_BUF_SIZE)) {
//     pod_->seed.PrintLine("selected file exceeds max file size limit. Closing file.");
//     return false;
//   }
//   else curr_header_.file_size = header.SubCHunk2Size;

//   /* NOTE: checking bit depth here in case it breaks */
//   if ((int)header.BitPerSample != BIT_DEPTH) {
//     pod_->seed.PrintLine("File has wrong bit depth. Closing file.");
//     return false;
//   }
//   else curr_header_.bit_depth = header.BitPerSample;

//   /* NOTE: below we find the length of the audio in samples.  
//     http://tiny.systems/software/soundProgrammer/WavFormatDocs.pdf */
//   curr_header_.num_samples = (header.SubCHunk2Size) / (header.NbrChannels*(header.BitPerSample/8));
//   f_lseek(curr_file_, sizeof(WAV_FormatTypeDef));

//   return true;
// }

bool AudioFileManager::LoadAudioData(){
  size_t samples_to_read = curr_header_.num_samples;
  size_t bytespersample = (curr_header_.bit_depth/8);
  size_t bytes_to_read = samples_to_read * sizeof(int16_t);
  size_t subchunkSize = curr_header_.file_size;
  pod_->seed.PrintLine("bytes to read: %d subchunk sz: %d", bytes_to_read, subchunkSize);

  if (samples_to_read > ABS_CHNL_BUF_SIZE){
    pod_->seed.PrintLine("selected file too large for buffer");
    return false;
  }
  
  const size_t chunk_size = 4096;
  int16_t temp_buff[chunk_size];
  size_t total_read = 0;
  size_t left_idx = 0;
  size_t right_idx = 0;

  while (total_read < samples_to_read){
    // get size of next chunk to be loaded
    size_t chunk_bytes = (samples_to_read < chunk_size) ? samples_to_read : chunk_size;

    UINT bytes_read;
    FRESULT res = f_read(curr_file_, temp_buff, chunk_bytes, &bytes_read);

    if (res != FR_OK || bytes_read != chunk_bytes){
      pod_->seed.PrintLine("error reading audio chunk: %d", res);
      return false;
    }

    size_t samples_in_chunk = bytes_read / sizeof(int16_t);

    if (curr_header_.channels==1){
      for (size_t i = 0; i< samples_in_chunk; i++){
        float sample = s162f(temp_buff[i]);
        left_channel[left_idx] = sample;
        left_idx++;
        right_channel[right_idx] = sample;
        right_idx++;
      }
    }
    else {
      for (size_t i = 0; i<samples_in_chunk; i+=2){
        left_channel[left_idx] = s162f(temp_buff[i]);
        left_idx++;
        right_channel[right_idx] = s162f(temp_buff[i+1]);
        right_idx++;
      }
    }
    total_read += samples_in_chunk;
  }
  pod_->seed.PrintLine("successfully loaded %zu samples", total_read);
  return true;
// }

// bool AudioFileManager::LoadAudioData() {
//     // Calculate total bytes to read
//     size_t bytes_per_sample = (curr_header_.bit_depth / 8) * curr_header_.channels;
//     size_t total_bytes = curr_header_.num_samples * bytes_per_sample;
    
//     pod_->seed.PrintLine("Total bytes to read: %d, File size: %d", total_bytes, curr_header_.file_size);

//     if (curr_header_.num_samples > ABS_CHNL_BUF_SIZE) {
//         pod_->seed.PrintLine("Selected file too large for buffer");
//         return false;
//     }

//     // Seek to the start of audio data
//     f_lseek(curr_file_, 44); // Standard WAV header size

//     const size_t chunk_size = 4096;
//     std::vector<uint8_t> temp_buff(chunk_size);
//     size_t total_read = 0;
//     size_t left_idx = 0;
//     size_t right_idx = 0;

//     while (total_read < total_bytes) {
//         size_t chunk_bytes = std::min(chunk_size, total_bytes - total_read);

//         UINT bytes_read;
//         FRESULT res = f_read(curr_file_, temp_buff.data(), chunk_bytes, &bytes_read);

//         if (res != FR_OK) {
//             pod_->seed.PrintLine("Error reading audio chunk: %d", res);
//             return false;
//         }

//         if (bytes_read != chunk_bytes) {
//             pod_->seed.PrintLine("Unexpected end of file. Read %d bytes, expected %d", bytes_read, chunk_bytes);
//             return false;
//         }

//         size_t samples_in_chunk = bytes_read / bytes_per_sample;

//         for (size_t i = 0; i < samples_in_chunk; i++) {
//             int16_t left_sample, right_sample;

//             if (curr_header_.channels == 1) {
//                 left_sample = right_sample = *reinterpret_cast<int16_t*>(&temp_buff[i * bytes_per_sample]);
//             } else {
//                 left_sample = *reinterpret_cast<int16_t*>(&temp_buff[i * bytes_per_sample]);
//                 right_sample = *reinterpret_cast<int16_t*>(&temp_buff[i * bytes_per_sample + 2]);
//             }

//             left_channel[left_idx++] = s162f(left_sample);
//             right_channel[right_idx++] = s162f(right_sample);
//         }

//         total_read += bytes_read;
//     }

//     pod_->seed.PrintLine("Successfully read %d bytes", total_read);
//     return true;
// }



bool AudioFileManager::GetWavHeader(FIL* file){
  WAV_FormatTypeDef header;
  UINT bytes_read;

  FRESULT res = f_read(file, &header, sizeof(WAV_FormatTypeDef),&bytes_read);
  if (res != FR_OK || bytes_read != sizeof(WAV_FormatTypeDef)){
    pod_->seed.PrintLine("failed to read header - check WAV file is valid");
    return false;
  }
  /* here we are checking each subchunk of the wav file is in the correct location and
      has the correct identifier - had some problems with weirdly formatted wav files
      nb: id values are in big endian */
  auto checkChunkId = [] (uint32_t chunkId, uint32_t targetVal){ return chunkId == targetVal; };
  bool checkRIFF = checkChunkId(header.ChunkId, 0x52494646);
  bool checkFmt = checkChunkId(header.SubChunk1ID, 0x666d7420);
  bool checkData = checkChunkId(header.SubChunk2ID, 0x64617461);

  if (checkRIFF && checkFmt && checkData){
    // correct format 
    curr_header_.sample_rate = header.SampleRate;
    curr_header_.channels = header.NbrChannels;
    curr_header_.bit_depth = header.BitPerSample;
    curr_header_.file_size = header.SubCHunk2Size;
  }
  // otherwise: data chunk may be misplaced, so search within file for it
  // ignore files with invalid riff/fmt header info - not compliant with WAV spec
  else if (checkRIFF && checkFmt){
    // data chunk might be misplaced
    curr_header_.sample_rate = header.SampleRate;
    curr_header_.channels = header.NbrChannels;
    curr_header_.bit_depth = header.BitPerSample;

    uint32_t chunk_id, chunk_size;
    // Skip to end of format header chunk, to start looking for data
    f_lseek(file, header.SubChunk1Size+12); 
    do {
      f_read(file, &chunk_id, 4, &bytes_read);
      f_read(file, &chunk_size, 4, &bytes_read);
      if (chunk_id == 0x64617461) {
        curr_header_.file_size = chunk_size;
        break;
      }
    } 
    while (bytes_read == 4);

    if (chunk_id != 0x64617461){
      pod_->seed.PrintLine("couldn't find data chunk - invalid WAV file");
      return false;
    }
  }
  else {
    pod_->seed.PrintLine("invalid WAV file - chunks may be misplaced");
    return false;
  }
    curr_header_.num_samples = curr_header_.file_size / (curr_header_.channels * (curr_header_.bit_depth / 8));
    pod_->seed.PrintLine("WAV file info:");
    pod_->seed.PrintLine("SR: %d, CHNLS: %d, Bits: %d, Samples: %d",
    curr_header_.sample_rate,  curr_header_.channels, curr_header_.bit_depth, curr_header_.num_samples);
    return true;
  }
}




bool AudioFileManager::CloseFile(){
  pod_->seed.Print("closing file: ");
  // curr_idx == index of current file
  pod_->seed.Print(names_[curr_idx_]);
  pod_->seed.PrintLine(" idx: %d "+ curr_idx_);
  return f_close(curr_file_) == FR_OK;
}

void AudioFileManager::SetBuffers(float *left, float *right, size_t buff_size){
  left_channel = left;
  right_channel = right;
  buff_size_ = buff_size;
}

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
