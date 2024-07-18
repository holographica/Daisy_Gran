#include <stdio.h>
#include "ff.h"
#include "daisy_pod.h"

using namespace daisy; 

class AudioFileManager {
  public:
    AudioFileManager(SdmmcHandler *sd, FatFSInterface *fsi, DaisyPod *pod)
                    : sd_(sd), fsi_(fsi), pod_(pod) {};
    void SetBuffers(float* left, float* right, size_t buff_size);
    bool Init();
    bool ScanWavFiles();
    bool LoadFile(int file_idx);
    bool GetWavHeader(FIL* file);
    bool CloseFile();
    void BlinkOnSDError (char type);
    


    // scan wav files fn from main file - returns list of filenames

    // load a file from sd card by filename/pointer to name - return bool

    // close file 

    // getters for wav file info

    // getter for pointer to audio buffer that file is loaded into
    // getter for buffer size?

    static const int MAX_FILES = 32;                      
    static const int MAX_FNAME_LEN = 128;
    static const int CHNL_BUF_SIZE = 16 * 1024 * 1024;
    static const int BIT_DEPTH = 16;
  private:
    struct WavHeader {
      int sample_rate;
      uint32_t file_size; // total size of wav file incl file/audio format info
      short channels;
      short bit_depth;
      uint32_t len_samples; // length of audio data in samples (per channel) ie no of samples/channel
    };

    SdmmcHandler* sd_;
    FatFSInterface* fsi_;
    DaisyPod* pod_;
    float* left_channel;
    float* right_channel;
    // list of filenames for logging/screen
    char names_ [MAX_FILES][MAX_FNAME_LEN];

    // index of currently selected file
    // NOTE: set to 0 here? or within init / scan func??
    int curr_idx_;
    // currently selected file
    FIL curr_file_; 

    // total no of files scanned
    // NOTE: 1 indexed not 0 indexed - do i need to do this?
    int file_count_;

    WavHeader curr_header_;



    // 32MB audio buffer for loading wav files
    // NOTE: can't use the macro here so need to declare this in main fn
    // float DSY_SDRAM_BSS wav_buffer_[MAX_BUFFER_SIZE];

  // file index
  // playback status? 
};
