#include "AudioFileManager.h"

using namespace daisy;

bool AudioFileManager::Init(){
  SdmmcHandler::Config sd_cfg;
  sd_cfg.Defaults();
  if (sd_.Init(sd_cfg) != SdmmcHandler::Result::OK) {
    return false;
  }

  fsi_.Init(FatFSInterface::Config::MEDIA_SD);
  if (f_mount(&fsi_.GetSDFileSystem(),"/",1) != FR_OK){
    return false;
  }
  return true;
}

/* Scan SD card for wavs and save filenames */
bool AudioFileManager::ScanWavFiles(){
  DIR dir;
  FILINFO fno;
  char name[MAX_FNAME_LEN];
  memset(names_, '\0', sizeof(names_));

  const char* path = fsi_.GetSDPath();
  if (f_opendir(&dir,path) != FR_OK){
    // log error message here
    return false;
  }

  int count=0;
  while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]!=0){
    if (!(fno.fattrib & (AM_HID|AM_DIR))){
      /* NOTE: below, we ignore AppleDouble metadata files starting with '._'
      so if a legit file starts with this prefix (hopefully not), it will be wrongly skipped */
      if (strncmp(fno.fname, "._",2)==0) continue;
      strcpy(name, fno.fname);      
      if (strstr(name, ".wav") || strstr(name, ".WAV")){
        strcpy(names_[count],name);
        count++;
      }
    } else {
      /* handle directories/hidden files */
      if (fno.fattrib & AM_DIR){
        continue;
      } else if (fno.fattrib & AM_HID){
        continue;
      }
    }
    if (count>=MAX_FILES-1) break;
  }
  f_closedir(&dir);
  file_count_ = count;
  curr_idx_ = 0;

  for (int i=0; i<count; i++){
    /* NOTE: this could poss break sth */
    if (names_[i][0]=='\0'){
      break;
    }

  }

  return (count!=0) ? true : false;
}

bool AudioFileManager::LoadFile(int sel_idx) {

// NOTE: changed from 1 indexing -> 0 index of file count
  if (sel_idx > file_count_) return false;

  if (sel_idx != curr_idx_) {
    f_close(curr_file_);
  }

  FRESULT open_res = f_open(curr_file_, names_[sel_idx], (FA_OPEN_EXISTING | FA_READ));
  if (open_res != FR_OK) {
    return false;
  }

  if (!GetWavHeader(curr_file_)){
    f_close(curr_file_);
    return false;
  }

  //TODO: CHECK IF SR = 48k? IF NOT RESAMPLE

  if (!LoadAudioData()) {

    return false;
  }

  return true;
}

bool AudioFileManager::CheckChunkID(uint32_t chunkId, uint32_t targetVal) {
    return chunkId == targetVal;
}

bool AudioFileManager::GetWavHeader(FIL* file){
  WAV_FormatTypeDef header;
  UINT bytes_read;
  FRESULT res = f_read(file, &header, sizeof(WAV_FormatTypeDef),&bytes_read);
  if (res != FR_OK || bytes_read != sizeof(WAV_FormatTypeDef)){

    return false;
  }
  /* here we check each subchunk of the wav file is in the correct location and
      has the correct identifier - had some problems with weirdly formatted wav files
      NB: id values here/below are in little endian, not big endian like they're supposed to be stored in wavs */
  bool checkRIFF = CheckChunkID(header.ChunkId, 0x46464952);
  bool checkWAV = CheckChunkID(header.FileFormat, 0x45564157);
  bool checkFmt = CheckChunkID(header.SubChunk1ID, 0x20746D66); 
  bool checkData = CheckChunkID(header.SubChunk2ID, 0x61746164);

  if (checkRIFF && checkWAV && checkFmt && checkData){
    // TODO: check if 48khz, if not then resample !!
    curr_header_.sample_rate = header.SampleRate;
    curr_header_.channels = header.NbrChannels;
    curr_header_.bit_depth = header.BitPerSample;
    curr_header_.file_size = header.SubCHunk2Size;
    /* below, we find the length of the audio in samples: 
    http://tiny.systems/software/soundProgrammer/WavFormatDocs.pdf */
    curr_header_.total_samples = curr_header_.file_size / (curr_header_.bit_depth / 8);
  }
  /* otherwise: data chunk may be misplaced, so search within file for it
  ignore files with invalid riff/fmt header info - not compliant with WAV spec */
  else if (checkRIFF && checkWAV && checkFmt){
    curr_header_.sample_rate = header.SampleRate;
    curr_header_.channels = header.NbrChannels;
    curr_header_.bit_depth = header.BitPerSample;

    uint32_t chunk_id, chunk_size;
    // skip to end of fmt chunk (byte 36) to start looking for data
    f_lseek(file, 36); 
    while (f_read(file, &chunk_id, 4, &bytes_read) == FR_OK && bytes_read == 4) {
      f_read(file, &chunk_size, 4, &bytes_read);
      
      char chunk_name[5] = {0};
      memcpy(chunk_name, &chunk_id, 4);
      // int fptr = f_tell(file);
  

      if (chunk_id == 0x61746164) {
        curr_header_.file_size = chunk_size;
        break;
      } 
      else {
        f_lseek(file, f_tell(file)+chunk_size);
      }
    }
    if (chunk_id != 0x61746164){
  
      return false;
    }
    curr_header_.total_samples = curr_header_.file_size / (curr_header_.bit_depth / 8);
  }

// TODO: CHANGE ABOVE SECTION SO I JUST CHECK THROUGH TO FIND FMT THEN DATA
// NOTE: FMT CHUNK CAN ALSO CHANGE POSITION
//       SO MAY AS WELL JUST CHECK FOR IT AS WELL? 

  else {
    if (!checkRIFF) 
    if (!checkWAV) 
    if (!checkFmt) 
    if (!checkData) 
    // NOTE: this logic fine? should be
    return false;
  }
  
  audio_data_start_ = f_tell(file); 
  FRESULT seek_res = f_lseek(curr_file_, audio_data_start_);
    if (seek_res != FR_OK) {
    
        return false;
    }
  return true;
}

bool AudioFileManager::LoadAudioData() {

  memset(left_channel_, 0.0f, sizeof(&left_channel_));
  memset(right_channel_, 0.0f, sizeof(&right_channel_));
  size_t total_samples = GetTotalSamples();
  size_t samples_per_channel = GetSamplesPerChannel();

  if (total_samples > ABS_CHNL_BUF_SIZE) {

    return false;
  }

  size_t chunk_size = 16384;
  alignas(16) std::vector<int16_t> temp_buff(chunk_size*curr_header_.channels);
  size_t samples_read = 0;
  UINT bytes_read;
  
  while (!f_eof(curr_file_) && samples_read < samples_per_channel){
    size_t samples_to_read = std::min(chunk_size, samples_per_channel - samples_read);
    FRESULT res = f_read(curr_file_, temp_buff.data(), samples_to_read * curr_header_.channels * sizeof(int16_t), &bytes_read);

    if (res != FR_OK) {
  
      return false;
    }
    /* nb: this is samples in chunk per channel */
    size_t samples_in_chunk = bytes_read / (curr_header_.channels * sizeof(int16_t));

    for (size_t i=0; i<samples_in_chunk; i++){
      if (curr_header_.channels==1){
        left_channel_[i+samples_read] = right_channel_[i+samples_read] = temp_buff[i];
      }
      else {
        left_channel_[i+samples_read] = temp_buff[i * 2];
        right_channel_[i+samples_read] = temp_buff[i * 2 + 1];
      }
    }
    samples_read += samples_in_chunk;
    // System::Delay(1);
  }
  return true;
}

bool AudioFileManager::CloseFile(){
  pod_.seed.Print("closing file: ");
  pod_.seed.Print(names_[curr_idx_]);
  return f_close(curr_file_) == FR_OK;
}

void AudioFileManager::SetBuffers(int16_t *left, int16_t *right){
  left_channel_ = left;
  right_channel_ = right;
}
