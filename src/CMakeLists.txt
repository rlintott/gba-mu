SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "-O3")

set(CAPSTONE_ZIP ../capstone-3.0.5-rc2.tar.gz)
set(CAPSTONE_DIR capstone-3.0.5-rc2)
set(CAPSTONE_LIB ${CAPSTONE_DIR}/libcapstone.a)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_DIR}
	COMMAND
        tar -xvf ${CMAKE_CURRENT_SOURCE_DIR}/${CAPSTONE_ZIP}
    COMMAND
        make -C ${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_DIR} CAPSTONE_STATIC=yes CAPSTONE_SHARED=no CAPSTONE_ARCHS="arm"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${CAPSTONE_ZIP}
)

find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)

add_library(gba_lib
    ARM7TDMI.cpp 
    util/static_for.h
    ARMInstructions/ArmDataProcHandler.cpp 
    ARMInstructions/ArmPsrHandler.cpp 
    ARMInstructions/ArmSdtHandler.cpp 
    ARMInstructions/ArmMultHandler.cpp 
    ARMInstructions/ArmHwdtHandler.cpp 
    ARMInstructions/ArmSdsHandler.cpp 
    ARMInstructions/ArmBdtHandler.cpp 
    ARMInstructions/ArmBranchHandler.cpp 
    ARMInstructions/ArmBranchXHandler.cpp 
    ARMInstructions/ArmSwiHandler.cpp 
    ARMInstructions/ArmUndefHandler.cpp
    ARM7TDMI.h
    Bus.cpp Bus.h
    GameBoyAdvance.cpp GameBoyAdvance.h
    Scheduler.cpp Scheduler.h
    LCD.cpp LCD.h
    PPU.cpp PPU.h
    Gamepad.cpp Gamepad.h
    DMA.cpp DMA.h
    Timer.cpp Timer.h
    Debugger.cpp Debugger.h
    )

set_property(SOURCE ARM7TDMI.h APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_DIR}/include)

target_link_libraries(gba_lib ${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_LIB})  
target_link_libraries(gba_lib sfml-graphics sfml-audio)  

add_executable(gba main.cpp)
add_executable(debug debug.cpp)

target_link_libraries(gba gba_lib)
target_link_libraries(debug gba_lib)

