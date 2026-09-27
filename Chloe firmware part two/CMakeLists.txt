cmake_minimum_required(VERSION 3.16)
project(firmware_part_two C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find IgH EtherCAT header and shared library on host system
find_path(ETHERCAT_INCLUDE_DIR ecrt.h PATHS /usr/include /usr/local/include /opt/etherlab/include)
find_library(ETHERCAT_LIBRARY ethercat PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /opt/etherlab/lib)

add_executable(ethercat_control main/main.c)

target_include_directories(ethercat_control PRIVATE ${ETHERCAT_INCLUDE_DIR})
target_link_libraries(ethercat_control PRIVATE ${ETHERCAT_LIBRARY} pthread rt)