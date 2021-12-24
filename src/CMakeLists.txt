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

    arm7tdmi/ArmInstructions/ArmDataProcHandler.cpp 
    arm7tdmi/ArmInstructions/ArmPsrHandler.cpp 
    arm7tdmi/ArmInstructions/ArmSdtHandler.cpp 
    arm7tdmi/ArmInstructions/ArmMultHandler.cpp 
    arm7tdmi/ArmInstructions/ArmHwdtHandler.cpp 
    arm7tdmi/ArmInstructions/ArmSdsHandler.cpp 
    arm7tdmi/ArmInstructions/ArmBdtHandler.cpp 
    arm7tdmi/ArmInstructions/ArmBranchHandler.cpp 
    arm7tdmi/ArmInstructions/ArmBranchXHandler.cpp 
    arm7tdmi/ArmInstructions/ArmSwiHandler.cpp 
    arm7tdmi/ArmInstructions/ArmUndefHandler.cpp

    arm7tdmi/ThumbInstructions/ThumbAddOffSpHandler.cpp        
    arm7tdmi/ThumbInstructions/ThumbCondBHandler.cpp           
    arm7tdmi/ThumbInstructions/ThumbLdStRegOffHandler.cpp      
    arm7tdmi/ThumbInstructions/ThumbMultLdStPushPopHandler.cpp
    arm7tdmi/ThumbInstructions/ThumbAddSubHandler.cpp          
    arm7tdmi/ThumbInstructions/ThumbImmHandler.cpp             
    arm7tdmi/ThumbInstructions/ThumbLdStSeBHwHandler.cpp       
    arm7tdmi/ThumbInstructions/ThumbRelAddrHandler.cpp
    arm7tdmi/ThumbInstructions/ThumbAluHandler.cpp             
    arm7tdmi/ThumbInstructions/ThumbLdPcRelHandler.cpp         
    arm7tdmi/ThumbInstructions/ThumbLdStSpRelHandler.cpp       
    arm7tdmi/ThumbInstructions/ThumbShiftHandler.cpp
    arm7tdmi/ThumbInstructions/ThumbBHandler.cpp               
    arm7tdmi/ThumbInstructions/ThumbLdStHwHandler.cpp          
    arm7tdmi/ThumbInstructions/ThumbLongBHandler.cpp           
    arm7tdmi/ThumbInstructions/ThumbSwiHandler.cpp
    arm7tdmi/ThumbInstructions/ThumbBxHandler.cpp              
    arm7tdmi/ThumbInstructions/ThumbLdStImmOffHandler.cpp      
    arm7tdmi/ThumbInstructions/ThumbMultLdStHandler.cpp        
    arm7tdmi/ThumbInstructions/ThumbUndefHandler.cpp

    arm7tdmi/ARM7TDMI.h
    memory/Bus.cpp memory/Bus.h
    memory/EEPROM.cpp memory/EEPROM.h
    GameBoyAdvanceImpl.cpp GameBoyAdvanceImpl.h
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

target_link_libraries(core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${CAPSTONE_LIB})  
target_link_libraries(core PUBLIC sfml-graphics sfml-audio)
target_link_libraries(gba_lib PRIVATE core)

add_executable(gba gba.cpp)
target_link_libraries(gba PUBLIC gba_lib)

