cmake_minimum_required(VERSION 3.13)
enable_testing()

file(GLOB sources "./*.cpp")
add_executable("${CMAKE_PROJECT_NAME}" ${sources})
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_options("${CMAKE_PROJECT_NAME}" BEFORE PRIVATE
        "-Wl,-subsystem,windows")
    target_link_options("${CMAKE_PROJECT_NAME}" BEFORE PRIVATE
        -static-libstdc++ -static-libgcc)
    target_link_libraries("${CMAKE_PROJECT_NAME}" mingw32 win_resource)
endif()
target_link_libraries("${CMAKE_PROJECT_NAME}"
    resource SDL2main SDL2 SDL2_ttf pthread)
install(TARGETS "${CMAKE_PROJECT_NAME}" DESTINATION "bin")
