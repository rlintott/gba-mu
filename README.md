# RyBoyAdvance

The RyBoyAdvance is a WIP Game Boy Advance emulator. It is written in C++ with an object-oriented design. There's still a lot of work to be done on it, and currently only a couple games are playable. 

## TODO:
- [ ] Optimization (currently runs at 170FPS on my laptop)
- [ ] Performant and accurate cycle counting / Gamepak prefetch
- [ ] Implement all PPU modes and features
- [ ] Getting a wide variety of games running
- [ ] Miscellaneous bugs 
- [ ] CPU instruction accuracy (pass armwrestler ROM)

![Alt text](media/kirby1.png?raw=true)
Still some graphical glitches....
![Alt text](media/kirby2.png?raw=true)
Kirby sucking!
## Building 
* **Dependencies:** cmake, c++17, sfml
* `cd build` `./build.sh`
* Has only been tested on MacOS, but feel free to try it on Linux and Windows and provide feedback
## Running
* **To run tests:** `cd build` `./build.sh` `ctest`
* **To run a ROM:** `cd build` `./gba <path_to_gba_rom>`
## Resources
Here are some resources I have been using to aid in developing this emulator
* [GBATek Documentation](https://problemkaputt.de/gbatek.htm#armcpureference)
* ARM7TDMI Data Sheet: ftp://ftp.dca.fee.unicamp.br/pub/docs/ea871/ARM/ARM7TDMIDataSheet.pdf
* [Emudev Discord Server](https://discord.gg/xxkAe5xm)

