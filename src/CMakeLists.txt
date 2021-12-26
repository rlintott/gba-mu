SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "-O3")

set(CAPSTONE_ZIP ../capstone-3.0.5-rc2.tar.gz)
set(CAPSTONE_DIR capstone-3.0.5-rc2)
set(CAPSTONE_LIB ${CAPSTONE_DIR}/libcapstone.a)
set(INCLUDE_DIR ../include)

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

set_property(SOURCE arm7tdmi/ARM7TDMI.h APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_DIR}/include)

target_link_libraries(core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_LIB})  
target_link_libraries(core PUBLIC sfml-graphics sfml-audio)
target_link_libraries(gba_lib PRIVATE core)

add_executable(gba gba.cpp)
target_link_libraries(gba PUBLIC gba_lib)

