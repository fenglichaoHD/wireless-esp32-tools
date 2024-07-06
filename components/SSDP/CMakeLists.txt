file(GLOB SOURCES ssdp.c
        )

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        PRIV_REQUIRES esp_netif memory_pool wt_storage
)
