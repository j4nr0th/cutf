cmake_minimum_required(VERSION 4.1)
project(cutf C)

set(CMAKE_C_STANDARD 23)

set(CUTF_SOURCE_FILES
    src/conv_basic.c
)

add_library(cutf STATIC ${CUTF_SOURCE_FILES})

if (CMAKE_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cutf PRIVATE -Wall -Wextra -fanalyzer -fpipe)
endif ()

set_property(TARGET cutf PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

enable_testing()
add_subdirectory(tests)

