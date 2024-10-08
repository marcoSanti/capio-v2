#####################################
# Project-wide settings
#####################################
cmake_minimum_required(VERSION 3.15)

# Define project name and description
project(capio-fs
        LANGUAGES CXX
        DESCRIPTION "Cross-Application Programmable I/O"
        VERSION 1.0.0
)

# Set required C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Generate the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Silence warning from clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -O0 -Wno-gnu-zero-variadic-macro-arguments")
endif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

#####################################
# Options
#####################################
option(CAPIO_BUILD_TESTS "Build CAPIO test suite" FALSE)
option(CAPIO_LOG "Enable capio debug logging" FALSE)
option(ENABLE_COVERAGE "Enable code coverage collection" FALSE)

#####################################
# CMake module imports
#####################################
include(FetchContent)
include(GNUInstallDirs)

#####################################
# Dependencies
#####################################
find_package(Threads REQUIRED)

#####################################
# Definitions
#####################################
add_compile_definitions(CAPIO_VERSION="${CMAKE_PROJECT_VERSION}")

IF (CAPIO_LOG)
    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling CAPIO logger")
        add_compile_definitions(CAPIO_LOG)
        execute_process(
                COMMAND bash "${PROJECT_SOURCE_DIR}/scripts/gen_syscallnames.sh"
                "${PROJECT_BINARY_DIR}/include/syscall/syscallnames.h"
        )
    ELSE (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "Capio logger enabled in release mode. skipping compilation of CAPIO logger")
    ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")
ENDIF (CAPIO_LOG)

#####################################
# Include files and directories
#####################################
file(GLOB_RECURSE CAPIO_COMMON_HEADERS "src/common/capio/*.hpp")
include_directories(src/common)

IF (CAPIO_LOG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    include_directories("${PROJECT_BINARY_DIR}/include/syscall")
ENDIF (CAPIO_LOG AND CMAKE_BUILD_TYPE STREQUAL "Debug")

#####################################
# Targets
#####################################
add_subdirectory(src/posix)
add_subdirectory(src/server)
add_subdirectory(src/capioctl)

IF (CAPIO_BUILD_TESTS)
    message(STATUS "Building CAPIO test suite")
    add_subdirectory(tests)
ENDIF (CAPIO_BUILD_TESTS)
