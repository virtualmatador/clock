cmake_minimum_required(VERSION 3.13)
enable_testing()

file(GLOB sources "./*.cpp")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(exe_type "WIN32")
else()
    set(exe_type "")
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable("${CMAKE_PROJECT_NAME}" ${exe_type} ${sources})
set(help_relative_path "../share/${CMAKE_PROJECT_NAME}.html")
cmake_path(NATIVE_PATH help_relative_path help_relative_path)
target_include_directories("${CMAKE_PROJECT_NAME}" PRIVATE
    ${SDL2_INCLUDE_DIR}
    ${SDL2_ttf_INCLUDE_DIR}
)
target_compile_definitions("${CMAKE_PROJECT_NAME}"
    PRIVATE PROJECT_VERSION="${CMAKE_PROJECT_VERSION}"
    PRIVATE HELP_RELATIVE_PATH=R"\(${help_relative_path}\)"
)
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
target_link_libraries("${CMAKE_PROJECT_NAME}"
    resource SDL2::SDL2main SDL2::SDL2 SDL2_ttf::SDL2_ttf)
install(TARGETS "${CMAKE_PROJECT_NAME}" DESTINATION "bin")
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
    install(FILES $<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}> TYPE BIN)
endif()
