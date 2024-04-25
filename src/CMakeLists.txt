cmake_minimum_required(VERSION 3.13)
enable_testing()

add_compile_definitions(PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
file(GLOB sources "./*.cpp")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(exe_type "WIN32")
else()
    set(exe_type "")
endif()
add_executable("${CMAKE_PROJECT_NAME}" ${exe_type} ${sources})
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PRIVATE
        HOME="HOME"
    )
    target_link_libraries("${CMAKE_PROJECT_NAME}" pthread)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PRIVATE
        HOME="USERPROFILE"
    )
    target_link_libraries("${CMAKE_PROJECT_NAME}" win_resource)
    set(dev_dir "C:/dev")
    target_include_directories("${CMAKE_PROJECT_NAME}" PRIVATE "${dev_dir}/include")
    target_link_directories("${CMAKE_PROJECT_NAME}" PRIVATE "${dev_dir}/lib")
    install(FILES "${dev_dir}/lib/SDL2.dll" DESTINATION "bin")
    install(FILES "${dev_dir}/lib/SDL2_ttf.dll" DESTINATION "bin")
endif()
target_link_libraries("${CMAKE_PROJECT_NAME}"
    resource SDL2main SDL2 SDL2_ttf)
install(TARGETS "${CMAKE_PROJECT_NAME}" DESTINATION "bin")
