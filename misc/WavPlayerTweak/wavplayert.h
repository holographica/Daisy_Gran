// Not written or owned by me:
// taken from a gist (https://gist.github.com/netherwaves/880785ccac5ddf5907ba187c16241ecc)
// by https://github.com/netherwaves

#pragma once
// #ifndef DSY_WAVPLAYER_H
// #define DSY_WAVPLAYER_H /**< Macro */
#include "daisy_core.h"
#include "util/wav_format.h"
#include "ff.h"

#define WAV_FILENAME_MAX \
    256 /**< Maximum LFN (set to same in FatFs (ffconf.h) */

namespace daisy
{
// TODO: add bitrate, samplerate, length, etc.
/** Struct containing details of Wav File. */
struct WavFileInfoT
{
    WAV_FormatTypeDef raw_data;               /**< Raw wav data */
    char              name[WAV_FILENAME_MAX]; /**< Wav filename */
    char              path[WAV_FILENAME_MAX]; /**< Wav filepath */
    uint32_t          length;                 /**< in samples   */
};

/* 
TODO:
- Make template-y to reduce memory usage.
*/


/** Wav Player that will load .wav files from an SD Card,
and then provide a method of accessing the samples with
double-buffering. */
class WavPlayerT
{
  public:
    WavPlayerT() {}
    ~WavPlayerT() {}

    /** Initializes the WavPlayer, loading up to max_files of wav files from an SD Card. */
    FRESULT Init(const char* search_path,
                 int16_t*    buffer,
                 size_t      bufferSize,
                 size_t      numChannels = 1);

    /** Opens the file at index sel for reading.
    \param sel File to open
     */
    int Open(size_t sel);

    /** Opens the file by name for reading.
    \param id ID of file to open
     */
    int OpenById(const char* id);

    /** Closes whatever file is currently open.
    \return &
     */
    int Close();

    /** \return The next sample if playing, otherwise returns 0 */
    int16_t  Stream();

    /** Collects buffer for playback when needed. */
    int Prepare();


    inline void Play()  { playing_ = true; };
    inline void Pause() { playing_ = false; };

    /** Resets the playback position to the beginning of the file immediately */
    void Restart();

    /** Sets whether or not the current file will repeat after completing playback. 
    \param loop To loop or not to loop.
    */
    inline void SetLooping(bool loop) { looping_ = loop; }

    /** \return Whether the WavPlayer is looping or not. */
    inline bool GetLooping() const { return looping_; }

    /** \return The number of files loaded by the WavPlayer */
    inline size_t GetNumberFiles() const { return file_cnt_; }

    /** \return currently selected file.*/
    inline size_t GetCurrentFile() const { return file_sel_; }

    inline const char* GetNameOf(const size_t idx_) const { return file_info_[idx_].name; }

    /** \return time left until the end of the playing file */
    uint32_t TimeUntilEOF();

  private:
    enum BufferState
    {
        BUFFER_STATE_IDLE,
        BUFFER_STATE_PREPARE_0,
        BUFFER_STATE_PREPARE_1,
    };

    BufferState GetNextBuffState();

    static constexpr size_t kMaxFiles = 26;

    int16_t*    buff_;
    size_t      bufferSize_;
    size_t      numChannels_;
    WavFileInfoT file_info_[kMaxFiles];
    size_t      file_cnt_, file_sel_;
    BufferState buff_state_;
    size_t      read_ptr_;
    uint32_t    read_ptr_abs_;
    bool        looping_, playing_;
    FIL         fil_;
};

} // namespace daisy

// #endif