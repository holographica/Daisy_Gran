// Not written or owned by me:
// taken from a gist (https://gist.github.com/netherwaves/880785ccac5ddf5907ba187c16241ecc)
// by https://github.com/netherwaves


#include <daisy_pod.h>
#include <daisysp.h>
#include "wavplayert.h"
#include "hidtweak.cpp"

using namespace daisy;
using namespace daisysp;

// define objects
DaisyPod hw;
SdmmcHandler sdcard;
FatFSInterface fsi;
WavPlayerT p1;
// WavPlayerT p2;

// define buffers
// these are initialized globally on the SDRAM memory sector
#define BUFSIZE 4096
int16_t DSY_SDRAM_BSS buffer1[BUFSIZE];
// int16_t DSY_SDRAM_BSS buffer2[BUFSIZE];

/**
 * @brief Audio callback
 * 
 * @param in input buffer
 * @param out output buffer
 * @param size block size
 */
void AudioCallback(AudioHandle::InterleavingInputBuffer in, AudioHandle::InterleavingOutputBuffer out, size_t size)
{
    float samp_out;

    // kfakfjghsjdfg
    for(size_t i = 0; i < size; i += 2)
    {
        // stream sampelrs
        samp_out  = s162f(p1.Stream());
        // samp_out += s162f(p2.Stream());

        out[i] = out[i+1] = samp_out * .25f;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(24);
    hw.seed.StartLog(true);

    System::Delay(1000);

    // initialize SD card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    // sd_cfg.speed = SdmmcHandler::Speed::FAST;
    sdcard.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    hw.seed.Print("mount...");
    if(FRESULT result = f_mount(&fsi.GetSDFileSystem(), fsi.GetSDPath(), 0))
    {
        hw.seed.PrintLine("did not mount - err %d", result);
        asm("bkpt 255");
    }
    hw.seed.PrintLine("ok!");
    System::Delay(1000);

    // initialize first sampler on /folder1
    if(int result = p1.Init("0:/", buffer1, BUFSIZE, 1))
    {
        hw.seed.PrintLine("did not initialize p1 - err %d", result);
        asm("bkpt 255");
    }
    hw.seed.PrintLine("p1 - loaded %d files", p1.GetNumberFiles());
    System::Delay(500);

    // // initialize second sampler on /folder2
    // if(int result = p2.Init("0:/folder2", buffer2, BUFSIZE, 1))
    // {
    //     hw.seed.PrintLine("did not initialize p2 - err %d", result);
    //     asm("bkpt 255");
    // }
    // hw.seed.PrintLine("p2 - loaded %d files", p2.GetNumberFiles());
    // System::Delay(500);
    
    // set both to looping
    p1.SetLooping(true);
    // p2.SetLooping(true);

    // open both samples respectively
    p1.Open(0);
    // p2.Open(0);

    // start audio
    hw.seed.Print("starting audio...");
    hw.StartAudio(AudioCallback);
    hw.seed.PrintLine("ok!");

    /**
     * MAIN LOOP
     */
    for(;;)
    {
        // prepare samples for sampler 1; manage errors
        if(int result = p1.Prepare())
        {
            hw.seed.PrintLine("ERROR WHILE PREPARING P1 - err %d", result);
            asm("bkpt 255");
        }
    int32_t inc;

    // Debounce digital controls
    hw.ProcessDigitalControls();

    // Change file with encoder.
    inc = hw.encoder.Increment();
    if(inc > 0) {
        size_t curfile;
        curfile = p1.GetCurrentFile();
        if(curfile < p1.GetNumberFiles() - 1)
        {
            p1.Open(curfile + 1);
            hw.seed.PrintLine("changed plus");
        }
    }
    else if(inc < 0) {
        size_t curfile;
        curfile = p1.GetCurrentFile();
        if(curfile > 0)
        {
            p1.Open(curfile - 1);
            hw.seed.PrintLine("changed minus");
        }
    }

        // prepare samples for sampler 2; manage errors
        // if(int result = p2.Prepare())
        // {
        //     hw.PrintLine("ERROR WHILE PREPARING P2 - err %d", result);
        //     asm("bkpt 255");
        // }
    }
}