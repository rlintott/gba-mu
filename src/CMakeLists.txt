include(FetchContent)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "-O3")

set(INCLUDE_DIR ../include)

find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)

add_library(gba_lib 
    ../include/GameBoyAdvance.hpp
    GameBoyAdvance.cpp)
target_include_directories(gba_lib INTERFACE ${INCLUDE_DIR})

add_library(core
    arm7tdmi/ARM7TDMI.cpp 
    util/static_for.h
    util/macros.h

    arm7tdmi/ARMInstructions/ArmDataProcHandler.h 
    arm7tdmi/ARMInstructions/ArmPsrHandler.h 
    arm7tdmi/ARMInstructions/ArmSdtHandler.h 
    arm7tdmi/ARMInstructions/ArmMultHandler.h 
    arm7tdmi/ARMInstructions/ArmHwdtHandler.h 
    arm7tdmi/ARMInstructions/ArmSdsHandler.h 
    arm7tdmi/ARMInstructions/ArmBdtHandler.h 
    arm7tdmi/ARMInstructions/ArmBranchHandler.h 
    arm7tdmi/ARMInstructions/ArmBranchXHandler.h 
    arm7tdmi/ARMInstructions/ArmSwiHandler.h 
    arm7tdmi/ARMInstructions/ArmUndefHandler.h

    arm7tdmi/ThumbInstructions/ThumbAddOffSpHandler.h        
    arm7tdmi/ThumbInstructions/ThumbCondBHandler.h           
    arm7tdmi/ThumbInstructions/ThumbLdStRegOffHandler.h      
    arm7tdmi/ThumbInstructions/ThumbMultLdStPushPopHandler.h
    arm7tdmi/ThumbInstructions/ThumbAddSubHandler.h          
    arm7tdmi/ThumbInstructions/ThumbImmHandler.h             
    arm7tdmi/ThumbInstructions/ThumbLdStSeBHwHandler.h       
    arm7tdmi/ThumbInstructions/ThumbRelAddrHandler.h
    arm7tdmi/ThumbInstructions/ThumbAluHandler.h             
    arm7tdmi/ThumbInstructions/ThumbLdPcRelHandler.h         
    arm7tdmi/ThumbInstructions/ThumbLdStSpRelHandler.h       
    arm7tdmi/ThumbInstructions/ThumbShiftHandler.h
    arm7tdmi/ThumbInstructions/ThumbBHandler.h               
    arm7tdmi/ThumbInstructions/ThumbLdStHwHandler.h          
    arm7tdmi/ThumbInstructions/ThumbLongBHandler.h          
    arm7tdmi/ThumbInstructions/ThumbSwiHandler.h
    arm7tdmi/ThumbInstructions/ThumbBxHandler.h              
    arm7tdmi/ThumbInstructions/ThumbLdStImmOffHandler.h      
    arm7tdmi/ThumbInstructions/ThumbMultLdStHandler.h        
    arm7tdmi/ThumbInstructions/ThumbUndefHandler.h

    arm7tdmi/ARM7TDMI.h

    memory/Bus.cpp memory/Bus.h
    memory/EEPROM.cpp memory/EEPROM.h
    memory/Flash.cpp memory/Flash.h

    GameBoyAdvanceImpl.cpp GameBoyAdvanceImpl.h
    Scheduler.cpp Scheduler.h
    LCD.cpp LCD.h
    PPU.cpp PPU.h
    Gamepad.cpp Gamepad.h
    DMA.cpp DMA.h
    Timer.cpp Timer.h
    Debugger.cpp Debugger.h
    )

FetchContent_Declare(capstone
    GIT_REPOSITORY https://github.com/capstone-engine/capstone/
    GIT_TAG 3.0.5-rc2)
FetchContent_GetProperties(capstone)
if(NOT capstone_POPULATED)
    FetchContent_Populate(capstone)
    set(CAPSTONE_STATIC ON)
    set(CAPSTONE_DYNAMIC OFF)
    set(CAPSTONE_ARCHS "arm")
    #See https://github.com/capstone-engine/capstone/commit/ce3f0d093548f2a04b9d5a021c983a53492ae740
    if(EXISTS ${capstone_SOURCE_DIR}/tests/test.c)
        file(RENAME ${capstone_SOURCE_DIR}/tests/test.c ${capstone_SOURCE_DIR}/tests/test_basic.c)
    endif()
    file(READ ${capstone_SOURCE_DIR}/CMakeLists.txt CAPSTONE_CMAKE_FILE)
    string(REPLACE "test.c" "test_basic.c" CAPSTONE_FIX ${CAPSTONE_CMAKE_FILE})
    file(WRITE ${capstone_SOURCE_DIR}/CMakeLists.txt ${CAPSTONE_FIX})
    add_subdirectory(${capstone_SOURCE_DIR} ${capstone_BINARY_DIR})
endif()

target_include_directories(core PRIVATE ${capstone_SOURCE_DIR}/include)

target_link_libraries(core PUBLIC sfml-graphics sfml-audio capstone-static)
target_link_libraries(gba_lib PRIVATE core)

add_executable(gba gba.cpp)
target_link_libraries(gba PUBLIC gba_lib)
