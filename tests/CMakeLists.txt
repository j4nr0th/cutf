add_executable(test_c8_to_c32 test_c8_to_c32.c)
target_link_libraries(test_c8_to_c32 PRIVATE cutf)
add_test(NAME c8toc32 COMMAND test_c8_to_c32)

add_executable(test_c32_to_c8 test_c32_to_c8.c)
target_link_libraries(test_c32_to_c8 PRIVATE cutf)
add_test(NAME c32toc8 COMMAND test_c32_to_c8)
