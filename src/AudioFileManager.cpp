#include <stdio.h>
#include "AudioFileManager.h"
#include <vector>

using namespace daisy;

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
    // TODO: change to enums here + above for sd error blink function
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


  file_count_ = count;
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

// NOTE: CHANGED THIS 
// NOTE: due to 1 indexing -> 0 index of file count
  if (sel_idx > file_count_) return false;

  pod_->seed.PrintLine("loading file: %s", names_[sel_idx]);
  if (sel_idx != curr_idx_) {
    pod_->seed.PrintLine("closing file: %d  opening: %d",curr_idx_, sel_idx);
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

bool AudioFileManager::GetWavHeader(FIL* file){
  WAV_FormatTypeDef header;
  UINT bytes_read;

  FRESULT res = f_read(file, &header, sizeof(WAV_FormatTypeDef),&bytes_read);
  if (res != FR_OK || bytes_read != sizeof(WAV_FormatTypeDef)){
    pod_->seed.PrintLine("failed to read header - check WAV file is valid");
    return false;
  }
  /* here we check each subchunk of the wav file is in the correct location and
      has the correct identifier - had some problems with weirdly formatted wav files
      NB: id values here/below are in little endian, not big endian like they're supposed to be stored in wavs */
  auto checkChunkId = [] (uint32_t chunkId, uint32_t targetVal){ return chunkId == targetVal; };
  bool checkRIFF = checkChunkId(header.ChunkId, 0x46464952);
  bool checkWAV = checkChunkId(header.FileFormat, 0x45564157);
  bool checkFmt = checkChunkId(header.SubChunk1ID, 0x20746D66); 
  bool checkData = checkChunkId(header.SubChunk2ID, 0x61746164);
  pod_->seed.PrintLine("data chunk id: %d",header.SubChunk2ID);
  pod_->seed.PrintLine("data chunk size: %d",header.SubCHunk2Size);

  if (checkRIFF && checkWAV && checkFmt && checkData){
    // TODO: check if 48khz, if not then resample !!
    curr_header_.sample_rate = header.SampleRate;
    curr_header_.channels = header.NbrChannels;
    curr_header_.bit_depth = header.BitPerSample;
    curr_header_.file_size = header.SubCHunk2Size;
    curr_header_.num_samples = curr_header_.file_size / (curr_header_.channels * (curr_header_.bit_depth / 8));
    pod_->seed.PrintLine("all chunks OK");
  }
  /* otherwise: data chunk may be misplaced, so search within file for it
  ignore files with invalid riff/fmt header info - not compliant with WAV spec */
  else if (checkRIFF && checkWAV && checkFmt){
    curr_header_.sample_rate = header.SampleRate;
    curr_header_.channels = header.NbrChannels;
    curr_header_.bit_depth = header.BitPerSample;
    pod_->seed.PrintLine("data chunk check failed");
    uint32_t chunk_id, chunk_size;
    // skip to end of fmt chunk, to start looking for data
    f_lseek(file, 36); 
    while (f_read(file, &chunk_id, 4, &bytes_read) == FR_OK && bytes_read == 4) {
      f_read(file, &chunk_size, 4, &bytes_read);
      
      char chunk_name[5] = {0};
      memcpy(chunk_name, &chunk_id, 4);
      // int fptr = f_tell(file);
      pod_->seed.PrintLine("Chunk: %s, Size: %lu bytes", chunk_name, chunk_size);

      if (chunk_id == 0x61746164) {
        curr_header_.file_size = chunk_size;
        break;
      } 
      else {
        f_lseek(file, f_tell(file)+chunk_size);
      }
    }
    if (chunk_id != 0x61746164){
      pod_->seed.PrintLine("couldn't find data chunk - invalid WAV file");
      return false;
    }
    curr_header_.num_samples = curr_header_.file_size / (curr_header_.channels * (curr_header_.bit_depth / 8));
  }
  else {
    if (!checkRIFF || !checkWAV){
      if (!checkRIFF) pod_->seed.PrintLine("RIFF header missing");
      if (!checkWAV) pod_->seed.PrintLine("wav header missing");
      pod_->seed.PrintLine("incorrect file type: requires WAVs only");
    }
    if (!checkFmt){
      pod_->seed.PrintLine("incorrect WAV structure: missing fmt chunk");
    }
    if (!checkData){
      pod_->seed.PrintLine("incorrect WAV structure: missing data chunk");
    }
    return false;
  }
  /* below, we find the length of the audio in samples: 
  http://tiny.systems/software/soundProgrammer/WavFormatDocs.pdf */
  pod_->seed.PrintLine("SR: %d, CHNLS: %d, Bits: %d, Samples: %d",
    curr_header_.sample_rate,  curr_header_.channels, curr_header_.bit_depth, curr_header_.num_samples);
  audio_data_start_ = f_tell(file); 
  pod_->seed.PrintLine("audio starts at idx %d", audio_data_start_);
  FRESULT seek_res = f_lseek(curr_file_, audio_data_start_);
    if (seek_res != FR_OK) {
        pod_->seed.PrintLine("Failed to seek to audio data start. Error: %d", seek_res);
        return false;
    }
  return true;
}


bool AudioFileManager::LoadAudioData() {
    // calculate total bytes to read
    size_t bytes_per_sample = (curr_header_.bit_depth / 8) * curr_header_.channels;
    size_t total_bytes = curr_header_.file_size;
    pod_->seed.PrintLine("Total bytes to read: %d", total_bytes);

    if (total_bytes > (ABS_CHNL_BUF_SIZE*2)) {
        pod_->seed.PrintLine("Selected file too large for buffer");
        return false;
    }

    // set file pointer to start of audio data
    // if (f_lseek(curr_file_, audio_data_start_) != FR_OK){
    if (f_lseek(curr_file_, 44) != FR_OK){
      pod_->seed.PrintLine("failed to seek to start of audio data");
      return false;
    }

    const size_t chunk_size = 4096;
    std::vector<uint8_t> temp_buff(chunk_size);
    size_t total_bytes_read = 0;
    // size_t left_idx = 0;
    // size_t right_idx = 0;
    size_t sample_idx = 0;

    while (total_bytes_read < total_bytes) {
        size_t bytes_left = total_bytes - total_bytes_read;
        size_t chunk_bytes = chunk_size < bytes_left? chunk_size : bytes_left;

        UINT bytes_read;
        FRESULT res = f_read(curr_file_, temp_buff.data(), chunk_bytes, &bytes_read);

        if (res != FR_OK) {
            pod_->seed.PrintLine("Error reading audio chunk: %d", res);
            return false;
        }

        if (bytes_read != chunk_bytes) {
            pod_->seed.PrintLine("Unexpected end of file. Read %d bytes, expected %d", bytes_read, chunk_bytes);
            return false;
        }

        size_t samples_in_chunk = bytes_read / bytes_per_sample;
        for (size_t i = 0; i < samples_in_chunk; i++) {
            int16_t left_sample, right_sample;
            size_t offset = i * bytes_per_sample;
            if (curr_header_.channels == 1) {
                left_sample = right_sample = *reinterpret_cast<int16_t*>(&temp_buff[offset]);
            } 
            else {
                left_sample = *reinterpret_cast<int16_t*>(&temp_buff[offset]);
                right_sample = *reinterpret_cast<int16_t*>(&temp_buff[offset + 2]);
            }
            left_channel[sample_idx] = s162f(left_sample);
            right_channel[sample_idx] = s162f(right_sample);
            sample_idx++;
            // left_channel[left_idx] = s162f(left_sample);
            // right_channel[right_idx] = s162f(right_sample);
            // left_idx++;
            // right_idx++;
        }

        total_bytes_read += bytes_read;
    }

    pod_->seed.PrintLine("Successfully read %d bytes", total_bytes_read);
    return true;
}



/* NOTE:modified old function */
// bool AudioFileManager::LoadAudioData() {
//   if (f_lseek(curr_file_, audio_data_start_) != FR_OK) {
//     pod_->seed.PrintLine("failed to seek to start of audio data");
//     return false;
//   }

//   size_t bytes_per_sample = (curr_header_.bit_depth / 8) * curr_header_.channels;
//   size_t total_bytes = curr_header_.num_samples * bytes_per_sample;

//   pod_->seed.PrintLine("Total bytes to read: %d, File size: %d, bps %d", total_bytes, curr_header_.file_size, bytes_per_sample);

//   if (total_bytes > ABS_CHNL_BUF_SIZE) {
//       pod_->seed.PrintLine("Selected file too large for buffer");
//       return false;
//   }

//   const size_t chunk_size = 4096;
//   /* here we use a vector as the dynamic allocation means the memory
//     segment is properly aligned: using a standard array doesn't work */
//   std::vector<uint8_t> temp_buf(chunk_size);
//   size_t total_bytes_read = 0;
//   // size_t sample_idx = 0;
//   size_t left_idx = 0;
//   size_t right_idx = 0;


//   while (total_bytes_read < total_bytes) {
//       size_t bytes_left = total_bytes - total_bytes_read;
//       // size_t chunk_bytes = std::min(chunk_size, total_bytes - total_bytes_read);
//       size_t chunk_bytes = chunk_size < bytes_left? chunk_size : bytes_left;

//       UINT bytes_read;
//       FRESULT res = f_read(curr_file_, temp_buf.data(), chunk_bytes, &bytes_read);
//       // pod_->seed.PrintLine("bytes read: %d chunk: %d", bytes_read, chunk_bytes);
//       if (res != FR_OK || bytes_read != chunk_bytes) {
//           pod_->seed.PrintLine("Error reading audio chunk: %d", res);
//           pod_->seed.PrintLine("Unexpected end of file. Read %d bytes, expected %d", bytes_read, chunk_bytes);
//           return false;
//       }

//       /* we are reading raw bytes from the file (uint8) but have to interpret them as int16 
//         (since they're stored as 16bit samples): so, we use reinterpret_cast to treat those 
//         bytes at that memory location as a diff type, without changing the underlying data */
//       // int16_t* sample_data = reinterpret_cast<int16_t*>(temp_buf.data());

//       size_t samples_in_chunk = bytes_read / bytes_per_sample;
//       for (size_t i = 0; i < samples_in_chunk; i++) {
//           int16_t left_sample, right_sample;
//           /* mono: each sample is 2 bytes so i+1 moves us 2 bytes to the next sample */
//           if (curr_header_.channels == 1) {
//             // float sample = s162f(sample_data[i]);
//             // left_channel[sample_idx] = right_channel[sample_idx] = sample;
//             left_sample = right_sample = *reinterpret_cast<int16_t*>(&temp_buf[i * bytes_per_sample]);
//           } 
//           else {
//             /* each pair LR of samples is 4 bytes. So each L or R sample is 4 bytes away from the next 
//             L or R sample. (i+1) moves us 2 bytes along: so i*2 to get to next L sample, (i*2)+1 to next R sample. 
//             for samples: (i,channel): 0,L -> 1,R -> 2,L -> 3,R 
//             for bytes: (offset,channel): 0,L -> 2,R -> 4,L -> 6,R */
//             // left_channel[sample_idx] = s162f(sample_data[i * 2]);
//             // right_channel[sample_idx] = s162f(sample_data[i * 2 + 1]);
//             left_sample = *reinterpret_cast<int16_t*>(&temp_buf[i * bytes_per_sample]);
           
//             right_sample = *reinterpret_cast<int16_t*>(&temp_buf[i * bytes_per_sample + 2]);
//           }
//           left_channel[left_idx++] = s162f(left_sample);
//           right_channel[right_idx++] = s162f(right_sample);
//       }
//       total_bytes_read += bytes_read;
//   }

//   pod_->seed.PrintLine("Successfully read %d bytes", total_bytes_read);
//   return true;
// }





/* NOTE: MY NEW FUNCTION */
// bool AudioFileManager::LoadAudioData(){
//   if (f_lseek(curr_file_, audio_data_start_) != FR_OK){
//     pod_->seed.PrintLine("failed to seek to start of audio data");
//     return false;
//   }

//   size_t samples_to_read = curr_header_.num_samples;
//   size_t bytes_to_read = curr_header_.channels * samples_to_read * sizeof(int16_t);
//   const size_t chunk_size = 4096;
//   // int16_t temp_buf[chunk_size];
//   std::vector<uint8_t>temp_buf(chunk_size);
//   size_t total_read = 0;
//   size_t left_idx = 0;
//   size_t right_idx = 0;

//   if (bytes_to_read > ABS_CHNL_BUF_SIZE){
//     pod_->seed.PrintLine("selected file too large for buffer");
//     return false;
//   }
//   int count =0;
//   while (total_read < samples_to_read){
//     pod_->seed.PrintLine("count: %d",count);
//     count++;
//     // get size of next chunk to be loaded
//     size_t chunk_bytes = (bytes_to_read < chunk_size) ? bytes_to_read : chunk_size;
//     pod_->seed.PrintLine("\nbytes to read: %d", bytes_to_read);
//     bytes_to_read -= chunk_bytes;
//     UINT bytes_read=0;
//     FRESULT res = f_read(curr_file_, temp_buf.data(), chunk_bytes, &bytes_read);

//     if (res != FR_OK || bytes_read != chunk_bytes){

//       pod_->seed.PrintLine("error reading audio chunk: %d", res);
//       pod_->seed.PrintLine("bytes_read: %d", bytes_read);
//       pod_->seed.PrintLine("chunk_bytes: %d", chunk_bytes);
//       return false;
//     }
    
//     size_t samples_in_chunk = bytes_read / sizeof(int16_t); 

//     if (curr_header_.channels==1){
//       for (size_t i = 0; i< samples_in_chunk; i++){
//         float sample = s162f(temp_buf[i]);
//         left_channel[left_idx] = sample;
//         left_idx++;
//         right_channel[right_idx] = sample;
//         right_idx++;
//       }
//     }
//     else {
//       for (size_t i = 0; i<samples_in_chunk; i+=2){
//         left_channel[left_idx] = s162f(temp_buf[i]);
//         left_idx++;
//         right_channel[right_idx] = s162f(temp_buf[i+1]);
//         right_idx++;
//       }
//     }
//     total_read += samples_in_chunk;
//   }
  
//   if (total_read != samples_to_read){
//     pod_->seed.PrintLine("read %zu samples: should have read %zu samples", total_read, samples_to_read);
//     return false;
//   }

//   pod_->seed.PrintLine("successfully loaded %zu samples", total_read);
//   return true;
// }

/* NOTE: MY NEW FUNCTION */
// bool AudioFileManager::GetWavHeader(FIL* file){
//   WAV_FormatTypeDef header;
//   UINT bytes_read;

//   FRESULT res = f_read(file, &header, sizeof(WAV_FormatTypeDef),&bytes_read);
//   if (res != FR_OK || bytes_read != sizeof(WAV_FormatTypeDef)){
//     pod_->seed.PrintLine("failed to read header - check that WAV file is valid");
//     return false;
//   }
//   /* here we check each subchunk of the wav file is in the correct location and
//       has the correct identifier - had some problems with weirdly formatted wav files
//       NB: id values are in big endian */
//   auto checkChunkId = [] (uint32_t chunkId, uint32_t targetVal){ return chunkId == targetVal; };
//   // bool checkRIFF = checkChunkId(header.ChunkId, 0x52494646);//big endian
//   bool checkRIFF = checkChunkId(header.ChunkId, 0x46464952); // little endian

//   pod_->seed.PrintLine("riff header: %d",header.ChunkId);
//   // bool checkWAV = checkChunkId(header.FileFormat, 0x57415645); // big endian
//   bool checkWAV = checkChunkId(header.FileFormat, 0x45564157); // little endian

//   pod_->seed.PrintLine("wav header: %d",header.FileFormat);
//   // bool checkFmt = checkChunkId(header.SubChunk1ID, 0x666d7420); // big endian
//   bool checkFmt = checkChunkId(header.SubChunk1ID, 0x20746D66); // little endian
//   pod_->seed.PrintLine("fmt header: %d",header.SubChunk1ID);
//   // bool checkData = checkChunkId(header.SubChunk2ID, 0x64617461); // big endian
//   bool checkData = checkChunkId(header.SubChunk2ID, 0x61746164); // little endian
//   pod_->seed.PrintLine("data header: %d",header.SubChunk2ID);

//   if (checkRIFF && checkWAV && checkFmt && checkData){
//     // TODO: check if 48khz, if not then resample !!
//     curr_header_.sample_rate = header.SampleRate;
//     curr_header_.channels = header.NbrChannels;
//     curr_header_.bit_depth = header.BitPerSample;
//     // TODO: check it fits in bufs - if not, close file? or just load as much as poss?
//     curr_header_.file_size = header.SubCHunk2Size;
//   }
//   /* otherwise: data chunk may be misplaced, so search within file for it
//   ignore files with invalid riff/fmt header info - not compliant with WAV spec */
//   else if (checkRIFF && checkWAV && checkFmt){
//     // data chunk might be misplaced
//     curr_header_.sample_rate = header.SampleRate;
//     curr_header_.channels = header.NbrChannels;
//     curr_header_.bit_depth = header.BitPerSample;

//     uint32_t chunk_id, chunk_size;
//     // skip to end of fmt chunk, to start looking for data
//     f_lseek(file, header.SubChunk1Size+12); 
//     do {
//       f_read(file, &chunk_id, 4, &bytes_read);
//       f_read(file, &chunk_size, 4, &bytes_read);
//       if (chunk_id == 0x64617461) {
//         curr_header_.file_size = chunk_size;
//         break;
//       }
//     } 
//     while (bytes_read == 4);

//     if (chunk_id != 0x64617461){
//       pod_->seed.PrintLine("couldn't find data chunk - invalid WAV file");
//       return false;
//     }
//   }
//   else {
//     if (!checkRIFF || !checkWAV){
//       if (!checkRIFF) pod_->seed.PrintLine("RIFF header missing");
//       if (!checkWAV) pod_->seed.PrintLine("wav header missing");
//       pod_->seed.PrintLine("incorrect file type: requires WAVs only");
//     }
//     if (!checkFmt){
//       pod_->seed.PrintLine("incorrect WAV structure: missing fmt chunk");
//     }
//     if (!checkData){
//       pod_->seed.PrintLine("incorrect WAV structure: missing data chunk");
//     }
//     return false;
//   }
//   /* below, we find the length of the audio in samples: 
//   http://tiny.systems/software/soundProgrammer/WavFormatDocs.pdf */
//   curr_header_.num_samples = curr_header_.file_size / (curr_header_.channels * (curr_header_.bit_depth / 8));
//   pod_->seed.Print("WAV file info: ");
//   pod_->seed.PrintLine("SR: %d, CHNLS: %d, Bits: %d, Samples: %d",
//     curr_header_.sample_rate,  curr_header_.channels, curr_header_.bit_depth, curr_header_.num_samples);
//   audio_data_start_ = f_tell(file); 
//   pod_->seed.PrintLine("audio start: %d", audio_data_start_);
//   FRESULT seek_res = f_lseek(curr_file_, audio_data_start_);
//     if (seek_res != FR_OK) {
//         pod_->seed.PrintLine("Failed to seek to audio data start. Error: %d", seek_res);
//         return false;
//     }
//   return true;
// }

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
