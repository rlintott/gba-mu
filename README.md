# Game Boy Advance Emulator (WIP)
Still very much a work in progress! Currently working through the TONC GBA programming tutorial demo roms. I aim to get all these working before trying to run an actual game.


## Progress
:white_circle:  CPU - Clean up / refactor code

:white_check_mark: CPU - 32bit ARM Instructions passing jsmolka's gba-suite test rom

:white_check_mark:  CPU - 16bit THUMB Instructions passing jsmolka's gba-suite test rom

:white_check_mark:  CPU - Cycle counting (sort of)

:white_circle: Gamepak Prefetch Buffer Emulation

:white_circle: Picture processing unit (PPU) / LCD screen

:white_circle: CPU - Interrupt handling

:white_circle: Timer Hardware

:white_check_mark: Bus - read / write capabilites

:white_check_mark: DMA (mostly, see TODOs)

:white_check_mark: `m3_demo.gba` TONC rom

:white_check_mark: `key_pad.gba` TONC rom

:white_check_mark: `dma_demo.gba` TONC rom

:white_check_mark: `win_demo.gba` TONC rom

:white_check_mark: `obj_demo.gba` TONC rom

:white_circle: Optimization (use profiler)

:white_circle: Everything else!


![Alt text](media/dma_demo.png?raw=true "Running TONC's dma demo")

![Alt text](media/obj_demo.png?raw=true "Running TONC's obj demo")

## Building 
* **Dependencies:** cmake, c++17, Curses, sfml
* `cd build` `./build.sh`

## Running
* **To run tests:** `cd build` `./build.sh` `ctest`

* **To run ROM:** `cd build` `./gba <path_to_rom>`


## Resources

Here are the main resources I am using to develop the emulator

* [GBATek Documentation](https://problemkaputt.de/gbatek.htm#armcpureference)
* ARM7TDMI Data Sheet: ftp://ftp.dca.fee.unicamp.br/pub/docs/ea871/ARM/ARM7TDMIDataSheet.pdf
* [Emudev Discord Server](https://discord.gg/xxkAe5xm)

