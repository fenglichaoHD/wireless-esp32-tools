file(GLOB SOURCES
        web_server.c
        web_uri_module.c
        uri_modules/uri_html_base.c
        uri_modules/uri_api.c
        )

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        REQUIRES esp_http_server api_router json
)

idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE ON)

