#This file included as test, having error with cirque_trackpad.h driver in include folder

zephyr_library()
zephyr_library_sources(cirque_trackpad.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/../include)