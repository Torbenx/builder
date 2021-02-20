cmake_minimum_required(VERSION 3.13)

set(source
    "main.cpp"
    "BuildRenderer.cpp"
    "FileExplorer.cpp"
    "HeadBar.cpp"
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_executable(builder ${source} ${platform_main_source})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_library(builder SHARED ${source} ${platform_main_source})
endif()

target_link_libraries(builder engine)