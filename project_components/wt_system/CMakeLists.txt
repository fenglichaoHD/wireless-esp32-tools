file(GLOB SOURCES *.c
        )

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        PRIV_REQUIRES
        global_resource esp_app_format api_router
)

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
add_compile_definitions(FW_UPD_DATE="${CURRENT_DATE}")
idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE ON)
