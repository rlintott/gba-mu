# Game Boy Advance Emulator (WIP)
Still very much a work in progress


## Progress
:white_check_mark: CPU - 32bit ARM Instructions passing jsmolka's gba-suite test rom

:white_check_mark:  CPU - 16bit THUMB Instructions passing jsmolka's gba-suite test rom

:white_circle:  CPU - Clean up / refactor code

:white_circle:  CPU - Cycle counting

:white_check_mark: Bus - basic read / write capabilites

:white_check_mark: Bus - can load a ROM. 

:white_circle:  Everything else



## Building 
* **Dependencies:** cmake, c++17, Curses
* `cd build` `./build.sh`

## Running
* **To run tests:** `cd build` `./build.sh` `ctest`


## Contributing

Thank you for wanting to contribute!

The emulator is a work in progress and there are many `TODO` comments scattered around the code, and there is alot to do besides that as well 

[Here](https://guides.github.com/introduction/flow/) is an overview of the workflow to use when contributing. 
 
1. Create a new feature branch off of current master
2. Implement your feature and test it
3. Create a PR back into master for review


## Resources

Here are the main resources I am using to develop the emulator

* [GBATek Documentation](https://problemkaputt.de/gbatek.htm#armcpureference)
* ARM7TDMI Data Sheet: ftp://ftp.dca.fee.unicamp.br/pub/docs/ea871/ARM/ARM7TDMIDataSheet.pdf
* [Emudev Discord Server](https://discord.gg/xxkAe5xm)

