SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

find_package(Curses REQUIRED)

add_library(gba_lib
    ARM7TDMI.cpp ArmOpcodeHandlers.cpp ARM7TDMI.h
    Bus.cpp Bus.h
    GameBoyAdvance.cpp GameBoyAdvance.h
    Debugger.cpp Debugger.h
    )
target_link_libraries(gba_lib ${CURSES_LIBRARIES})
    
add_executable(gba main.cpp)
target_link_libraries(gba gba_lib)
