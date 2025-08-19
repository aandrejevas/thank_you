cmake_minimum_required(VERSION 4.1)

project(main VERSION 1.0 LANGUAGES CXX)

include(C:/msys64/home/aandr/maker/default.cmake)

set_target_properties(${TARGET} PROPERTIES WIN32_EXECUTABLE True)
target_link_libraries(${TARGET} PRIVATE stdc++exp SDL3 SDL3_ttf SDL3_image)
