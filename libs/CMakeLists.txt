cmake_minimum_required(VERSION 3.18)

set(ENABLE_STATIC OFF CACHE BOOL "override")
add_subdirectory(libjpeg-turbo)

add_subdirectory(fmt)
