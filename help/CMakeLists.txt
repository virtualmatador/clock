cmake_minimum_required(VERSION 3.13)

add_custom_target("helper" ALL DEPENDS "${CMAKE_PROJECT_NAME}.html")
add_custom_command(OUTPUT "${CMAKE_PROJECT_NAME}.html"
    COMMAND ${CMAKE_COMMAND} -E cat "${CMAKE_CURRENT_SOURCE_DIR}/header.html" > "${CMAKE_PROJECT_NAME}.html"
    COMMAND ${CMAKE_COMMAND} -E cat "${CMAKE_CURRENT_SOURCE_DIR}/help.html" >> "${CMAKE_PROJECT_NAME}.html"
    COMMAND ${CMAKE_COMMAND} -E cat "${CMAKE_CURRENT_SOURCE_DIR}/footer.html" >> "${CMAKE_PROJECT_NAME}.html"
    DEPENDS "header.html" "help.html" "footer.html")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.html" DESTINATION "share")
