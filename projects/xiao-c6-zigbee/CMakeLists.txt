cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PARTITION_CSV "${CMAKE_SOURCE_DIR}/partitions.csv")
project(xiao_c6_zigbee)
