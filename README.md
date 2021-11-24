# RyBoyAdvance

The RyBoyAdvance is a WIP Game Boy Advance emulator. It is written in C++ with an object-oriented design. There's still a lot of work to be done on it, and currently only a couple games are playable. 

## Upcoming Features and TODO:
- [ ] Optimization (RyBoyAdvance currently runs around 170FPS on my laptop)
- [ ] Unique API for debugging / live rom hacking
- [ ] Performant and accurate cycle counting / Gamepak prefetch emulation
- [ ] All PPU modes and features
- [ ] Getting more games running
- [ ] Miscellaneous bugs 
- [ ] Improved CPU instruction accuracy
- [ ] Sound controller

Still some graphical glitches....
![Alt text](media/kirby1.png?raw=true)
![Alt text](media/kirby2.png?raw=true)

## Building 
* **Dependencies:** cmake, c++17, sfml
* `cd build` `./build.sh`
* Has only been tested on MacOS, but feel free to try it on Linux and Windows and provide feedback
## Running
* **To run tests:** `cd build` `./build.sh` `ctest`
* **To run a ROM:** `cd build` `./gba <path_to_gba_rom>`
## Resources
Here are some resources I have been using to aid in developing this emulator
* [GBATek Documentation](https://problemkaputt.de/gbatek.htm#armcpureference) is the authoritative reference for GBA's hardware
* ARM7TDMI Data Sheet: ftp://ftp.dca.fee.unicamp.br/pub/docs/ea871/ARM/ARM7TDMIDataSheet.pdf CPU reference sheet
* [Emudev Discord Server](https://discord.gg/xxkAe5xm) a welcoming and helpful EmuDev community

