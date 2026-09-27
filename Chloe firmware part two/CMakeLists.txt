cmake_minimum_required(VERSION 3.16)
project(firmware_part_two C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find IgH EtherCAT header and shared library on host system
find_path(ETHERCAT_INCLUDE_DIR ecrt.h PATHS /usr/include /usr/local/include)
find_library(ETHERCAT_LIBRARY ethercat PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)

if (NOT ETHERCAT_INCLUDE_DIR)
    message(WARNING "ecrt.h not found in system include paths.")
endif()

add_executable(ethercat_control main/main.c)

target_include_directories(ethercat_control PRIVATE ${ETHERCAT_INCLUDE_DIR})
target_link_libraries(ethercat_control PRIVATE ${ETHERCAT_LIBRARY} pthread rt)