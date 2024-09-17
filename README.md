### nb: full usage guide/docs and standalone binary coming soon!

---


## Installation

If this is your first time programming the Daisy Pod, visit the Daisy wiki at https://github.com/electro-smith/DaisyWiki. Follow the instructions to download the toolchain, which is needed for the build process (and further development, if you fancy it). 

Once installed:

1. Create a new directory and clone this repo:
```
mkdir new_folder || cd new_folder
git clone https://github.com/holographica/Daisy_Gran.git
```

2. Build the libraries and the application. This may take a minute or two:
```
cd Daisy_Gran
make
```

3. Connect your Pod and flash the program:
```
make program-dfu
```
  
4. Insert an SD card or external audio input to get started with the synth!
