file(GLOB SOURCES *.c
        )

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        PRIV_REQUIRES
        global_resource esp_app_format api_router
)

# Execute the Git command to get the formatted commit date
execute_process(
        COMMAND git show -s --format=%cd --date=format:%Y-%m-%d
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE FM_DATE
        RESULT_VARIABLE result
        ERROR_VARIABLE git_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT "${result}" STREQUAL "0")
    message(WARNING "Git command failed with: ${git_error}")
endif()

message(STATUS FM_DATE:${FM_DATE})

target_compile_definitions(${COMPONENT_LIB} PRIVATE FW_UPD_DATE="${FM_DATE}")
idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE ON)
