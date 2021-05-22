add_executable(test_arm testArm.cpp)
target_link_libraries(test_arm gba_lib)
add_test(test_arm test_arm)

configure_file(arm.log arm.log COPYONLY)