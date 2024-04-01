file(GLOB SOURCES
        web_server.c
        web_uri_module.c
        uri_modules/uri_ws.c
        uri_modules/uri_api.c
        uri_modules/uri_html_base.c
        )

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        REQUIRES esp_http_server
        PRIV_REQUIRES request_runner api_router json static_buffer utils html
)

idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE ON)

