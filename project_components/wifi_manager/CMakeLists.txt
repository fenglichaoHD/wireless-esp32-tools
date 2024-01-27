file(GLOB SOURCES
        wifi_event_handler.c
        wifi_manager.c
        )


idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        PRIV_REQUIRES mdns esp_wifi esp_event
)