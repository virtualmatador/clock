cmake_minimum_required(VERSION 3.13)

set(BASE_DIR "hicolor")
set(RESOLUTIONS 16 22 32 48 256 512)
foreach(RESOLUTION ${RESOLUTIONS})
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${BASE_DIR}/${RESOLUTION}x${RESOLUTION}/apps")
    set(OUTPUT_PNG "${OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.png")
    add_custom_command(OUTPUT "${OUTPUT_PNG}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
        COMMAND inkscape -e "${OUTPUT_PNG}" -w ${RESOLUTION} -h ${RESOLUTION} "${CMAKE_CURRENT_SOURCE_DIR}/icon.svg"
        MAIN_DEPENDENCY "icon.svg")
    add_custom_target("${RESOLUTION}-png" ALL DEPENDS "${OUTPUT_PNG}")
    list(APPEND png_targets "${RESOLUTION}-png")
    list(APPEND png_files "${OUTPUT_PNG}")
endforeach()
add_custom_target("app_icon" ALL DEPENDS "app.ico")
add_custom_command(OUTPUT "app.ico"
    COMMAND convert ${png_files} "app.ico" 
    DEPENDS ${png_targets})
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES ${png_files} DESTINATION "share/icons/${OUTPUT_DIR}")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_library("win_resource")
    set_target_properties("win_resource" PROPERTIES LINKER_LANGUAGE CXX)
    add_custom_command(OUTPUT "win_resource.o"
        COMMAND ${CMAKE_COMMAND} -E echo "108 ICON \\\"app.ico\\\"" > "res.rc"
        COMMAND "${CMAKE_RC_COMPILER}" -i "res.rc" -O coff -o "win_resource.o"
        DEPENDS "app_icon")
    target_sources("win_resource" PRIVATE "win_resource.o")
    SET_SOURCE_FILES_PROPERTIES("win_resource.o"
        PROPERTIES EXTERNAL_OBJECT true GENERATED true)
    target_link_libraries("win_resource" INTERFACE
        -Wl,--whole-archive
        "${CMAKE_CURRENT_BINARY_DIR}/win_resource.o"
        -Wl,--no-whole-archive)
endif()