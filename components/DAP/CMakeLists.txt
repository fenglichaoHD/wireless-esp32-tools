file(GLOB SOURCES
        ./cmsis-dap/source/*.c
        )

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
)
