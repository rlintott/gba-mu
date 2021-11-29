# RyBoyAdvance

The RyBoyAdvance is a WIP Game Boy Advance emulator. It is written in C++ with an object-oriented design. There's still a lot of work to be done on it, and currently only a couple games are playable. 

Now that most of the system is emulated, I am focusing on improving performance and refactoring the code and architecture before moving on to making the emulator more accurate. 

## Upcoming Features and TODOa:
- [ ] Code refactoring 
- [ ] Optimization (RyBoyAdvance currently runs around 170FPS on my laptop)
- [ ] Debugging mode
- [ ] Performant and accurate cycle counting / Gamepak prefetch emulation
- [ ] All PPU modes and features
- [ ] Getting more games running
- [ ] Miscellaneous bugs 
- [ ] Improved CPU instruction accuracy
- [ ] Sound controller

![Alt text](media/kirby.png?raw=true)
![Alt text](media/street.png?raw=true)

## Building 
* **Dependencies:** cmake, c++17, sfml
* `cd build` `./build.sh`
* Has only been tested on MacOS, but feel free to try it on Linux and Windows and provide feedback
## Running
* **To run tests:** `cd build` `./build.sh` `ctest`
* **To run a ROM:** `cd build` `./gba <path_to_gba_rom>`
## Resources
Here are some resources I have been using to aid in developing this emulator
* [GBATek Documentation](https://problemkaputt.de/gbatek.htm#armcpureference) is an authoritative reference for GBA's hardware
* [TONC GBA Programming guide](https://www.coranac.com/tonc/text/) is a great resouce that provides a lot of demo ROMs, indispensible for testing various components.
* ARM7TDMI Data Sheet: ftp://ftp.dca.fee.unicamp.br/pub/docs/ea871/ARM/ARM7TDMIDataSheet.pdf CPU reference sheet
* [Emudev Discord Server](https://discord.gg/xxkAe5xm) a welcoming and helpful EmuDev community

