file(GLOB SOURCES *.c)

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        PRIV_REQUIRES mdns esp_wifi esp_event
)