cmake_minimum_required(VERSION 3.27)
project(cp LANGUAGES C CXX)

# enable_testing()

set(TARGET cp)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
add_executable(${TARGET} main.cpp)

# add_subdirectory(tests)

target_compile_features(${TARGET} PUBLIC cxx_std_20)
target_compile_options(
    ${TARGET} PRIVATE 
    -Wall -Wpedantic -Wextra -Wshadow -Wno-sign-compare 
    -fsanitize=signed-integer-overflow -fsanitize=pointer-overflow -fsanitize=shift
    -fsanitize=address -fsanitize=bounds
    -fsanitize=integer-divide-by-zero -fsanitize=float-divide-by-zero
    -fsanitize=return -fsanitize=null
    -fsanitize=undefined
)

target_link_options(
    ${TARGET} PRIVATE 
    -fsanitize=address
    -fsanitize=bounds
)