file(GLOB SOURCES
        wifi_event_handler.c
        wifi_manager.c
        wifi_api_json.c
        wifi_api.c
        wifi_json_utils.c
        wifi_storage.c
        )


idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        PRIV_REQUIRES mdns esp_wifi esp_event api_router wt_storage driver SSDP
)

idf_component_set_property(${COMPONENT_NAME} WHOLE_ARCHIVE ON)
