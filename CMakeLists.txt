cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

project("clock" VERSION 1.1.0)
set(CMAKE_PROJECT_DESCRIPTION "Digital Clock with Chimes and Alarms")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://www.shaidin.com/clock")

set(CPACK_PACKAGE_VENDOR "virtualmatador")
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/license.txt")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_BINARY_DIR}/icon/app.ico")
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libsdl2-2.0-0 (>= 2.0.0), libsdl2-ttf-2.0-0 (>= 2.0.0)")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_UPGRADE_GUID "5A33D85D-5591-48ED-AD10-E7DB2B8552A7")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY
        "${CPACK_PACKAGE_VENDOR}\\\\${CMAKE_PROJECT_NAME}")
    set(CPACK_WIX_PRODUCT_ICON "${CPACK_PACKAGE_ICON}")
endif()
include(CPack)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        install(FILES "clock.desktop" DESTINATION "share/applications")
endif()

add_custom_target("conf" ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/.clock.conf")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/.clock.conf"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/.clock.conf"
        "${CMAKE_CURRENT_BINARY_DIR}/.clock.conf"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/.clock.conf"
)

add_custom_target("version" ALL
    COMMAND ${CMAKE_COMMAND} -E echo ${CMAKE_PROJECT_VERSION} >
        "${CMAKE_CURRENT_BINARY_DIR}/version.txt"
)

add_subdirectory("res")
add_subdirectory("icon")
add_subdirectory("src")
add_subdirectory("help")
