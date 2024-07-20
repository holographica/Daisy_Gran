#pragma once

#include <stdio.h>
#include "ff.h"
#include "diskio.h"
#include "daisy_pod.h"

using namespace daisy; 

class AudioFileManager {
  public:
    AudioFileManager(SdmmcHandler *sd, FatFSInterface *fsi, DaisyPod *pod, FIL *file)
      : sd_(sd), fsi_(fsi), pod_(pod), curr_file_(file), left_channel(nullptr), right_channel(nullptr), buff_size_(0) {};
    
    bool Init();
    bool ScanWavFiles();
    void SetBuffers(float *left, float *right, size_t buff_size);
    bool LoadFile(int file_idx);
    bool CloseFile();
    void BlinkOnSDError (char type);  
    bool GetWavHeader(FIL *file);

    float* GetLeftBuffer() const { return left_channel; }
    float* GetRightBuffer() const { return right_channel; }
    size_t GetBufferSize() const { return buff_size_; } // do i need this? 
    int GetSampleRate() const { return curr_header_.sample_rate; }
    uint32_t GetNumSamples() const { return curr_header_.num_samples; }
    int GetNumChannels() const { return curr_header_.channels; }

    static const int MAX_FILES = 32;                      
    static const int MAX_FNAME_LEN = 128;
    static const int ABS_CHNL_BUF_SIZE = 16 * 1024 * 1024;
    static const int BIT_DEPTH = 16;
    
  private:
    struct WavHeader {
      int sample_rate;
      uint32_t file_size; // total size of audio data in wav file excl file/audio format info
      short channels;
      short bit_depth;
      uint32_t num_samples; // no of samples/channel ie length of audio data in samples per channel
    };

    bool LoadAudioData();
    /// resample function? 

    SdmmcHandler* sd_;
    FatFSInterface* fsi_;
    DaisyPod* pod_;
    FIL* curr_file_; 
    float* left_channel;

    float* right_channel;
    size_t buff_size_;
    // list of filenames for logging/screen
    char names_ [MAX_FILES][MAX_FNAME_LEN];

    // index of currently selected file
    // NOTE: set to 0 here? or within init / scan func??
    int curr_idx_;

    // total no of files scanned
    // NOTE: 1 indexed not 0 indexed - do i need to do this?
    int file_count_;

    WavHeader curr_header_;


  // playback status? 
};
