cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Proyecto)

list(APPEND EXTRA_COMPONENT_DIRS
/home/ubuntu/esp/esp-idf/examples/common_components/protocol_examples_common)