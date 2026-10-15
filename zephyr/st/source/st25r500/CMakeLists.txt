# SPDX-License-Identifier: Apache-2.0

zephyr_compile_definitions("-DST25R500")

zephyr_library()
zephyr_library_sources(
        rfal_rfst25r500.c
        st25r500.c
        st25r500_com.c
        st25r500_dpocr.c
        st25r500_irq.c
)
zephyr_include_directories(.)
