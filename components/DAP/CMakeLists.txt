file(GLOB SOURCES
        ./cmsis-dap/source/DAP.c
        ./cmsis-dap/source/DAP_vendor.c
        ./cmsis-dap/source/JTAG_DP.c
        ./cmsis-dap/source/SW_DP.c
        ./cmsis-dap/source/SWO.c
        ./cmsis-dap/source/spi_op.c
        ./cmsis-dap/source/spi_switch.c
        ./cmsis-dap/source/dap_utility.c
        )

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
)
