# Game Boy Advance Emulator (WIP)
Still very much a work in progress


## Progress
:white_circle:  CPU - Clean up / refactor code

:white_check_mark: CPU - 32bit ARM Instructions passing jsmolka's gba-suite test rom

:white_check_mark:  CPU - 16bit THUMB Instructions passing jsmolka's gba-suite test rom

:white_check_mark:  CPU - Cycle counting

:white_circle: Gamepak Prefetch Buffer Emulation

:white_circle: Picture processing unit (PPU) / LCD screen

:white_circle: CPU - Interrupt handling

:white_check_mark: Bus - basic read / write capabilites

:white_check_mark: Bus - can load a ROM. 

:white_check_mark: `m3_demo.gba` TONC rom

:white_circle: Everything else


![Alt text](media/m3_demo.png?raw=true "Running TONC's m3 demo")


## Building 
* **Dependencies:** cmake, c++17, Curses, sfml
* `cd build` `./build.sh`

## Running
* **To run tests:** `cd build` `./build.sh` `ctest`


## Resources

Here are the main resources I am using to develop the emulator

* [GBATek Documentation](https://problemkaputt.de/gbatek.htm#armcpureference)
* ARM7TDMI Data Sheet: ftp://ftp.dca.fee.unicamp.br/pub/docs/ea871/ARM/ARM7TDMIDataSheet.pdf
* [Emudev Discord Server](https://discord.gg/xxkAe5xm)

