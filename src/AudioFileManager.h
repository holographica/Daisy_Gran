#pragma once
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "daisy_pod.h"
#include "constants_utils.h"
#include "debug_print.h"

using namespace daisy; 

class AudioFileManager {
  public:
    AudioFileManager(SdmmcHandler &sd, FatFSInterface &fsi, DaisyPod &pod, FIL *file)
      : sd_(sd), fsi_(fsi), pod_(pod), curr_file_(file), 
        left_buf_(nullptr), right_buf_(nullptr) {};
    
    bool Init();
    bool ScanWavFiles();
    void SetBuffers(int16_t *left, int16_t *right);
    bool LoadFile(uint16_t file_idx);
    
    bool CloseFile();
    bool GetWavHeader(FIL *file);
    bool CheckChunkID(uint32_t chunk_id, uint32_t target_val);

    int16_t* GetLeftBuffer() const { return left_buf_; }
    int16_t* GetRightBuffer() const { return right_buf_; }
    size_t GetSamplesPerChannel() const { return curr_header_.total_samples / curr_header_.channels; }
    size_t GetTotalSamples() const { return curr_header_.total_samples; }
    int16_t GetNumChannels() const { return curr_header_.channels; }
    uint16_t GetFileCount() const { return file_count_; }
    void GetName(uint16_t idx, char* name) const { strcpy(name, names_[idx]); }

  private:
    bool LoadAudioData();
    /* helper functions for loading WAV audio based on bit depth */
    bool Load16BitAudio(size_t samples_per_channel);
    // bool Load24BitAudio(size_t samples_per_channel); // TODO: shelved for later if time

    /* helper functions for loading audio */
    size_t GetSamplesInChunk(UINT bytes_read, size_t bytes_per_sample);
    size_t GetBytesPerSample();

    struct WavHeader {
      int sample_rate;
      /* total size of audio data in wav file excl file/audio format info */
      uint32_t file_size; 
      int16_t channels;
      int16_t bit_depth;
      size_t total_samples;
    };

    /* references to hardware interfaces */
    SdmmcHandler& sd_;
    FatFSInterface& fsi_;
    DaisyPod& pod_;
    FIL* curr_file_; 
    /* pointers to master left/right channel buffers */
    int16_t* left_buf_;
    int16_t* right_buf_;
    /* list of filenames for logging */
    char names_ [MAX_FILES][MAX_FNAME_LEN];
    /* index of currently selected file */
    uint16_t curr_idx_;
    uint16_t file_count_;
    /* header data for currently selected file */
    WavHeader curr_header_;
    /* byte in original wav file at which audio samples start - usually 44 */
    size_t audio_data_start_;
    /* chunk size for reading audio into temporary buffer */
    const size_t BUF_CHUNK_SZ = 16384;
};
