#pragma once

#include <stdio.h>
#include <stdint.h>
#include <vector>
// #include "ff.h"
// #include "diskio.h"
#include "daisy_pod.h"
#include "audio_constants.h"

using namespace daisy; 

class AudioFileManager {
  public:
    AudioFileManager(SdmmcHandler *sd, FatFSInterface *fsi, DaisyPod *pod, FIL *file)
      : sd_(sd), fsi_(fsi), pod_(pod), curr_file_(file), left_channel(nullptr), right_channel(nullptr) {};
    
    bool Init();
    bool ScanWavFiles();
    void SetBuffers(int16_t *left, int16_t *right);
    bool LoadFile(int file_idx);
    bool CloseFile();
    void BlinkOnSDError (char type);  
    bool GetWavHeader(FIL *file);
    bool CheckBufferIntegrity();
    bool CheckChunkID(uint32_t chunkId, uint32_t targetVal);

    int16_t* GetLeftBuffer() const { return left_channel; }
    int16_t* GetRightBuffer() const { return right_channel; }
    size_t GetSamplesPerChannel() const { return curr_header_.total_samples / curr_header_.channels; }
    size_t GetTotalSamples() const { return curr_header_.total_samples; }
    int GetNumChannels() const { return curr_header_.channels; }
    int GetFileCount() const { return file_count_; }
    void GetName(int idx, char* name) const { strcpy(name, names_[idx]); }

    static const int MAX_FILES = 32;                      
    static const int MAX_FNAME_LEN = 128;

  private:
    struct WavHeader {
      int sample_rate;
      uint32_t file_size; // total size of audio data in wav file excl file/audio format info
      int16_t channels;
      int16_t bit_depth;
      size_t total_samples;
    };

    bool LoadAudioData();
    /// resample function? 

    SdmmcHandler* sd_;
    FatFSInterface* fsi_;
    DaisyPod* pod_;
    FIL* curr_file_; 
    // pointers to master left/right channel buffers
    int16_t* left_channel;
    int16_t* right_channel;
    // list of filenames for logging/screen
    char names_ [MAX_FILES][MAX_FNAME_LEN];
    // index of currently selected file
    int curr_idx_;
    int file_count_;
    // header data for currently selected file
    WavHeader curr_header_;
    // byte in original wav file at which audio samples start
    int audio_data_start_;
    bool change_file_;
};
