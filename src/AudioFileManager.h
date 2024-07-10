class AudioFileManager {
  public:
    AudioFileManager(SdmmcHandler* sd, FatFSInterface* fsi, DaisyPod* pod):
                      sd_(sd), fsi_(fsi), pod_(pod){};
    bool Init();
    bool ScanWavFiles();
    bool LoadFile(int file_idx);
    bool CloseFile();


    // scan wav files fn from main file - returns list of filenames

    // load a file from sd card by filename/pointer to name - return bool

    // close file 

    // getters for wav file info

    // getter for pointer to audio buffer that file is loaded into
    // getter for buffer size?

    static const int MAX_FILES = 32;                      
    static const int MAX_FNAME_LEN = 128;
    static const int MAX_BUFFER_SIZE = 32 * 1024 * 1024;
  private:
    SdmmcHandler* sd_;
    FatFSInterface* fsi_;
    DaisyPod* pod_;
    // list of filenames for logging/screen
    char names_ [MAX_FILES][MAX_FNAME_LEN];
    // index of currently selected file
    int curr_idx_;
    // currently selected file
    FIL curr_file_; 
    // total no of files scanned
    // NOTE: 1 indexed not 0 indexed - do i need to do this?

    int file_count_;
    // 32MB audio buffer
    float DSY_SDRAM_BSS aubuffer_[MAX_BUFFER_SIZE];


  // wav file info header struct
  // file index
  // playback status? 
};
