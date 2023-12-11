cmake_minimum_required(VERSION 3.10)
project(Conundrum64)

set(CMAKE_C_STANDARD 99) # Adjust the standard as needed
set(CMAKE_C_COMPILER clang) # or another C compiler like clang
# Set the compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wfloat-equal -O2 -march=native -fstack-protector-strong -D_FORTIFY_SOURCE=2 -g -Wno-variadic-macros -Wno-gnu-zero-variadic-macro-arguments")


set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")

# Find SDL2 package
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Include SDL2 and subdirectories headers
include_directories(${SDL2_INCLUDE_DIRS} src src/ux src/inih src/c64)

# Automatically include all .c and .h files in the /src directory and its subdirectories
file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE HEADERS "src/*.h")

# Create an executable with the sourced files
add_executable(con64 ${SOURCES} ${HEADERS})

# Link SDL2 libraries
target_link_libraries(con64 "/Library/Frameworks/SDL2.framework" "/Library/Frameworks/SDL2_ttf.framework")
