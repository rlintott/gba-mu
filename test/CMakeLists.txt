add_executable(test_arm testArm.cpp)
add_executable(test_thumb testThumb.cpp)

target_link_libraries(test_arm gba_lib)
add_test(test_arm test_arm)

target_link_libraries(test_thumb gba_lib)
add_test(test_thumb test_thumb)

configure_file(arm.log arm.log COPYONLY)
configure_file(arm.gba arm.gba COPYONLY)
configure_file(thumb.log thumb.log COPYONLY)
configure_file(thumb.gba thumb.gba COPYONLY)