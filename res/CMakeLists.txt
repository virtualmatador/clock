cmake_minimum_required(VERSION 3.13)

add_library("resource")
set_target_properties("resource" PROPERTIES LINKER_LANGUAGE CXX)
file(GLOB RESOURCES "*.ttf")
foreach(RESOURCE ${RESOURCES})
	get_filename_component(RESOURCE_NAME "${RESOURCE}" NAME)
	configure_file("${RESOURCE}" "${RESOURCE_NAME}" COPYONLY)
	add_custom_command(OUTPUT "${RESOURCE_NAME}.o"
		COMMAND ${CMAKE_LINKER} -r -b binary "${RESOURCE_NAME}" -o "${RESOURCE_NAME}.o"
		MAIN_DEPENDENCY "${RESOURCE_NAME}")
	target_sources("resource" PRIVATE "${RESOURCE_NAME}.o")
	SET_SOURCE_FILES_PROPERTIES("${RESOURCE_NAME}.o"
		PROPERTIES EXTERNAL_OBJECT true GENERATED true)
endforeach()
