file(GLOB SOURCES wt_nvs.c wt_storage.c)

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        PRIV_REQUIRES nvs_flash
)