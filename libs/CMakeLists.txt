cmake_minimum_required(VERSION 3.18)

set(ENABLE_STATIC FALSE)
add_subdirectory(libjpeg-turbo)
unset(ENABLE_STATIC)
