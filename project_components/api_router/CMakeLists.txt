file(GLOB SOURCES *.c
        )


idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        REQUIRES json request_runner wt_common
        PRIV_REQUIRES memory_pool
)
