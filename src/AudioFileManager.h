#pragma once
#include <stdio.h>
#include <stdint.h>
#include "daisy_pod.h"
#include "constants_utils.h"
#include "debug_print.h"
#include "wavheader.h"

using namespace daisy; 

class AudioFileManager {
  public:
    // AudioFileManager(SdmmcHandler &sd, FatFSInterface &fsi, DaisyPod &pod, FIL *file)
    //   : sd_(sd), fsi_(fsi), pod_(pod), curr_file_(file){};
    AudioFileManager(): pod_(nullptr) {};
    
    void Init(SdmmcHandler& sd, FatFSInterface& fsi, DaisyPod& pod, FIL* file);
    bool ScanWavFiles();
    void SetBuffers(int16_t *left, int16_t *right);
    bool LoadFile(uint16_t file_idx);
    bool CloseFile();
    bool GetWavHeader(FIL *file);

    int16_t* GetLeftBuffer() const { return left_buf_; }
    int16_t* GetRightBuffer() const { return right_buf_; }
    std::size_t GetSamplesPerChannel() const { return header_.total_samples / header_.channels; }
    std::size_t GetTotalSamples() const { return header_.total_samples; }
    int16_t GetNumChannels() const { return header_.channels; }
    uint16_t GetFileCount() const { return file_count_; }
    void GetName(uint16_t idx, char* name) const { strcpy(name, names_[idx]); }

  private:
    /* methods for loading WAV audio data */
    bool LoadAudioData();
    bool Load16BitAudio(std::size_t samples_per_channel);

    /* references to hardware interfaces */
    SdmmcHandler* sd_;
    FatFSInterface* fsi_;
    DaisyPod* pod_;
    FIL* curr_file_; 
    /* pointers to master left/right channel buffers */
    static int16_t* left_buf_;
    static int16_t* right_buf_;
    /* list of filenames for logging */
    static DTCMRAM_BSS char names_ [MAX_FILES][MAX_FNAME_LEN];
    /* index of currently selected file */
    static DTCMRAM_BSS uint16_t curr_idx_;
    static DTCMRAM_BSS uint16_t file_count_;
    /* header data for currently selected file */
    static DTCMRAM_BSS WavHeader header_;
    /* byte in original wav file at which audio samples start - usually 44 */
    static DTCMRAM_BSS uint8_t audio_data_start_;
    /* chunk size for reading audio into temporary buffer */
};
