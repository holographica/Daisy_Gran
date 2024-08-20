#include "AudioFileManager.h"

using namespace daisy;

/// @brief Initialises sd card and file system interfaces
/// @return True if initialisation succeeds, else false
bool AudioFileManager::Init(){
  SdmmcHandler::Config sd_cfg;
  sd_cfg.Defaults();
  if (sd_.Init(sd_cfg) != SdmmcHandler::Result::OK) {
    DebugPrint(pod_,"SD init failed.");
    return false;
  }
  fsi_.Init(FatFSInterface::Config::MEDIA_SD);
  DebugPrint(pod_, "FSI init ok.");
  if(f_mount(&fsi_.GetSDFileSystem(),"/",1) != FR_OK){
    DebugPrint(pod_, "failed to mount FSI");
    return false;
  }
  else {
    DebugPrint(pod_, "mounted FSI ok.");
    return true;
  }
}

/// @brief Scans for list of WAVs on SD card and stores filenames
/// @return False if directory fails to open or there are no valid files, else true
bool AudioFileManager::ScanWavFiles(){
  DIR dir;
  FILINFO fno;
  // char name[MAX_FNAME_LEN];
  memset(names_, '\0', sizeof(names_));

  if (f_opendir(&dir,fsi_.GetSDPath()) != FR_OK){
    DebugPrint(pod_, "failed to open SD card directory");
    return false;
  }
  /* loop over files in SD card root directory */
  while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]!=0 && file_count_ < MAX_FILES){
    /* skip hidden files / directories */
    if (!(fno.fattrib & (AM_HID|AM_DIR))){
      /* ignore AppleDouble metadata files starting with '._'  */
      if (strncmp(fno.fname, "._",2)==0) continue;
      // strcpy(name, fno.fname);      
      // if (strstr(name, ".wav") || strstr(name, ".WAV")){
      if (strstr(fno.fname, ".wav") || strstr(fno.fname, ".WAV")){
        strcpy(names_[file_count_],fno.fname);
        names_[file_count_][MAX_FNAME_LEN-1] = '\0';
        file_count_++;
      }
    }
  }
  f_closedir(&dir);
  DebugPrint(pod_, "%d files found",file_count_);
  return file_count_ > 0;
}

/// @brief Opens a WAV file, gets its header data and loads the audio data
/// @param sel_idx The index of the selected file in the list of files on the SD card
/// @return True if file audio data is loaded succesfully
/// @return False if file fails to load - this could be because:
///         - File index is invalid
///         - File can't be opened from SD card
///         - Bit depth or sample rate values are not supported 
///         - Audio data fails to load 
bool AudioFileManager::LoadFile(uint16_t sel_idx) {
  if (sel_idx > file_count_) return false;
  if (sel_idx != curr_idx_) {
    f_close(curr_file_);
  }
  if(f_open(curr_file_, names_[sel_idx], (FA_OPEN_EXISTING | FA_READ))!=FR_OK){
    DebugPrint(pod_, "FatFS failed to open file");
    return false;
  } 

  if (!GetWavHeader(curr_file_)){
    f_close(curr_file_);
    DebugPrint(pod_, "failed to parse wav header ");
    return false;
  }
  if (header_.bit_depth != 16 || header_.sample_rate!=48000){
    DebugPrint(pod_, "wrong file format");
    return false;
  }

  return LoadAudioData();
}

/// @brief Parses audio format data from a WAV file header
/// @param file The WAV file to be parsed
/// @return True if all data is successfully parsed
/// @return False if file can't be read or data is missing
bool AudioFileManager::GetWavHeader(FIL* file){
  WAV_FormatTypeDef header;
  UINT bytes_read;
  FRESULT res = f_read(file, &header, sizeof(WAV_FormatTypeDef),&bytes_read);
  if (res != FR_OK || bytes_read != sizeof(WAV_FormatTypeDef)){
    return false;
  }
  /* here we check each subchunk of the wav file is in the correct location and
    has the correct identifier - had some problems with weirdly formatted wav files
    NB: id values here/below are in little endian (usually big endian) */
  bool checkRIFF = (header.ChunkId == 0x46464952);
  bool checkWAV = (header.FileFormat == 0x45564157);
  bool checkFmt = (header.SubChunk1ID == 0x20746D66);
  bool checkData = (header.SubChunk2ID == 0x61746164);

  /* ignore files with invalid riff/fmt header info - not compliant with WAV spec */
  if (!checkRIFF || !checkWAV){ return false; }
  if (checkFmt){
    header_.sample_rate = header.SampleRate;
    header_.channels = header.NbrChannels;
    header_.bit_depth = header.BitPerSample;
  }
  if (checkData){
    header_.file_size = header.SubCHunk2Size;
  /* find audio len in samples: http://tiny.systems/software/soundProgrammer/WavFormatDocs.pdf */
    header_.total_samples = header_.file_size / (header_.bit_depth/8);
  }
  /* otherwise: data chunk may be misplaced, so search within file for it */
  if (!checkFmt || !checkData){
    uint32_t chunk_id, chunk_size;
    f_lseek(file, 12); /* skip to usual start of fmt chunk (byte 12) to start looking */
    while (f_read(file, &chunk_id, 4, &bytes_read) == FR_OK && bytes_read == 4) {
      f_read(file, &chunk_size, 4, &bytes_read);
      
      if (chunk_id==0x20746D66) { /* this is 'fmt' in little endian */
        
        f_lseek(file, f_tell(file)+6); /* skip chunk size and audio format - 6 bytes */
        /* now get num of channels - 2 bytes*/
        if (f_read(file, &header_.channels, 2, &bytes_read)!=FR_OK || bytes_read!=2){
          return false;
        }
        /* get sample rate - 4 bytes */
        if (f_read(file, &header_.sample_rate, 4, &bytes_read)!=FR_OK || bytes_read!=4){
          return false;
        }
        f_lseek(file, f_tell(file)+6); /* skip byte rate and block align - 6 bytes */
        /* finally get bit depth - 2 bytes */
        if (f_read(file, &header_.bit_depth, 2, &bytes_read)!=FR_OK || bytes_read!=2){
          return false;
        }
        checkFmt = true;
      }
      else if (chunk_id==0x61746164){ /* this is 'data' in little endian */
        header_.file_size = chunk_size;
        checkData = true;
        break;
      } 
      else {
        f_lseek(file, f_tell(file)+chunk_size);
      }
      if (checkFmt && checkData) break;
    }
  }
  if (!checkFmt || !checkData) return false;
  header_.total_samples = header_.file_size / (header_.bit_depth / 8);
  audio_data_start_ = f_tell(file); 
  FRESULT seek_res = f_lseek(curr_file_, audio_data_start_);
  return seek_res == FR_OK;
}

/// @brief Clear data buffers, check file length is within bounds, call audio loader
/// @return True if audio is loaded, false if file is too long or incorrect bit depth
bool AudioFileManager::LoadAudioData() {
  memset(left_buf_, 0.0f, CHNL_BUF_SIZE_ABS);
  memset(right_buf_, 0.0f, CHNL_BUF_SIZE_ABS);
  size_t samples_per_channel = GetSamplesPerChannel();

  if (samples_per_channel > CHNL_BUF_SIZE_SAMPS) {
    return false;
  }
  return Load16BitAudio(samples_per_channel);
}

/// @brief Reads chunks of bytes from audio file into temporary buffer, then into SDRAM
/// @param samples_per_channel Length of audio in samples, per channel 
/// @return True if audio is loaded. False if file fails to read
bool AudioFileManager::Load16BitAudio(size_t samples_per_channel){
  size_t samples_read = 0;
  UINT bytes_read;

  std::vector<int16_t> temp_buff(BUF_CHUNK_SZ);


  while (!f_eof(curr_file_) && samples_read<samples_per_channel){
    size_t samples_to_read = std::min(BUF_CHUNK_SZ, (samples_per_channel-samples_read));
    size_t bytes_to_read = samples_to_read * header_.channels * sizeof(int16_t);
    if (f_read(curr_file_, temp_buff.data(), bytes_to_read, &bytes_read)!=FR_OK){
      DebugPrint(pod_, "failed to read file from SD card");
      return false;
    }

    size_t samples_in_chunk = bytes_read/(header_.channels*sizeof(int16_t));
    for (size_t i=0; i<samples_in_chunk; i++){
      if (header_.channels == 1){
        left_buf_[i+samples_read] = right_buf_[i+samples_read] = temp_buff[i];
      }
      else {
        left_buf_[i+samples_read] = temp_buff[i * 2];
        right_buf_[i+samples_read] = temp_buff[i * 2 + 1];
      }
    }
    samples_read += samples_in_chunk;
  }
  return true;
}

/// @brief Closes currently open file
/// @return True if file was successfully closed, else false
bool AudioFileManager::CloseFile(){
  DebugPrint(pod_, "Closing file: %d",names_[curr_idx_]);
  return f_close(curr_file_) == FR_OK;
}

/// @brief Assign the locations of the audio data buffers
/// @param left Pointer to the left channel buffer
/// @param right Pointer to the right channel buffer
void AudioFileManager::SetBuffers(int16_t *left, int16_t *right){
  left_buf_ = left;
  right_buf_ = right;
}
