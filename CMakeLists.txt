cmake_minimum_required(VERSION 3.16)

project(deprecatos C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_ASM_COMPILER gcc)

set(OBJECT_FLAGS "-m32 -ffreestanding -fno-pie -fno-stack-protector")
set(CMAKE_C_FLAGS "${OBJECT_FLAGS} -O0 -g3 -Wall -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "${OBJECT_FLAGS} -g3")

set(CMAKE_EXE_LINKER_FLAGS "-m32 -nostdlib -no-pie -static -T ${CMAKE_CURRENT_SOURCE_DIR}/linker_script.ld -Wl,--gc-sections -Wl,--build-id=none")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

include_directories(
include
src
)

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.s")

add_executable(${PROJECT_NAME}.elf ${SOURCES})
