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

    ArmInstructions/ArmDataProcHandler.cpp 
    ArmInstructions/ArmPsrHandler.cpp 
    ArmInstructions/ArmSdtHandler.cpp 
    ArmInstructions/ArmMultHandler.cpp 
    ArmInstructions/ArmHwdtHandler.cpp 
    ArmInstructions/ArmSdsHandler.cpp 
    ArmInstructions/ArmBdtHandler.cpp 
    ArmInstructions/ArmBranchHandler.cpp 
    ArmInstructions/ArmBranchXHandler.cpp 
    ArmInstructions/ArmSwiHandler.cpp 
    ArmInstructions/ArmUndefHandler.cpp

    ThumbInstructions/ThumbAddOffSpHandler.cpp        
    ThumbInstructions/ThumbCondBHandler.cpp           
    ThumbInstructions/ThumbLdStRegOffHandler.cpp      
    ThumbInstructions/ThumbMultLdStPushPopHandler.cpp
    ThumbInstructions/ThumbAddSubHandler.cpp          
    ThumbInstructions/ThumbImmHandler.cpp             
    ThumbInstructions/ThumbLdStSeBHwHandler.cpp       
    ThumbInstructions/ThumbRelAddrHandler.cpp
    ThumbInstructions/ThumbAluHandler.cpp             
    ThumbInstructions/ThumbLdPcRelHandler.cpp         
    ThumbInstructions/ThumbLdStSpRelHandler.cpp       
    ThumbInstructions/ThumbShiftHandler.cpp
    ThumbInstructions/ThumbBHandler.cpp               
    ThumbInstructions/ThumbLdStHwHandler.cpp          
    ThumbInstructions/ThumbLongBHandler.cpp           
    ThumbInstructions/ThumbSwiHandler.cpp
    ThumbInstructions/ThumbBxHandler.cpp              
    ThumbInstructions/ThumbLdStImmOffHandler.cpp      
    ThumbInstructions/ThumbMultLdStHandler.cpp        
    ThumbInstructions/ThumbUndefHandler.cpp

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

