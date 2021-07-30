SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

find_package(Curses REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)

add_library(gba_lib
    ARM7TDMI.cpp ArmOpcodeHandlers.cpp ThumbOpcodeHandlers.cpp ARM7TDMI.h
    Bus.cpp Bus.h
    GameBoyAdvance.cpp GameBoyAdvance.h
    Debugger.cpp Debugger.h
    LCD.cpp LCD.h
    )
target_link_libraries(gba_lib ${CURSES_LIBRARIES})
target_link_libraries(gba_lib sfml-graphics sfml-audio)  

add_executable(gba main.cpp)
target_link_libraries(gba gba_lib)
