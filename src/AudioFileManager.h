class AudioFileManager {
  public:
    AudioFileManager();
    bool Init();
    // TODO: return list
    bool AudioFileManager::ScanWavFiles();
    // scan wav files fn from main file - returns list of filenames

    // load a file from sd card by filename/pointer to name - return bool

    // close file 

    // getters for wav file info

    // getter for pointer to audio buffer that file is loaded into
    // getter for buffer size?
  private:
    SdmmcHandler sd_;
    FatFSInterface fsi_;

  // wav file info header struct
  // list of wav file names
  // file index
  // current file
  // audio buffer? 
  // playback status? 
};
