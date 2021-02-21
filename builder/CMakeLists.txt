cmake_minimum_required(VERSION 3.13)

set(source
    "main.cpp"
    "BuildRenderer.cpp"
    "FileExplorer.cpp"
    "HeadBar.cpp"
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_executable(builder ${source} ${PLATFORM_MAIN_SOURCE})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_library(builder SHARED ${source} ${PLATFORM_MAIN_SOURCE})
endif()

target_link_libraries(builder engine)