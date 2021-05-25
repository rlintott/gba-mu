# Game Boy Advance Emulator (WIP)
Still very much a work in progress


## Progress
:white_check_mark: CPU - 32bit ARM Instructions passing jsmolka's gba-suite test rom
- [ ]  CPU - 16bit THUMB Instructions passing jsmolka's gba-suite test rom
- [ ]  CPU - Clean up / refactor code
- [ ]  CPU - Cycle counting
:white_check_mark: Bus - basic read / write capabilites
:white_check_mark: Bus - can load a ROM. 
- [ ]  Everything else

## Todos
There are many `TODO` comments scattered around the code, feel free to implement any of them and make a PR!

## Building 
* **Dependencies:** cmake, c++17, Curses
* `cd build` `./build.sh`

## Running
* **To run tests:** `cd build` `./build.sh` `ctest`
