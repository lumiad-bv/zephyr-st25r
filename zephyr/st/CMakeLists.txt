# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
        source/rfal_analogConfig.c
        source/rfal_cd.c
        source/rfal_crc.c
        source/rfal_dlma.c
        source/rfal_dpo.c
        source/rfal_iso15693_2.c
        source/rfal_isoDep.c
        source/rfal_nfc.c
        source/rfal_nfca.c
        source/rfal_nfcb.c
        source/rfal_nfcDep.c
        source/rfal_nfcf.c
        source/rfal_nfcv.c
        source/rfal_st25tb.c
        source/rfal_st25xv.c
        source/rfal_t1t.c
        source/rfal_t2t.c
        source/rfal_t4t.c
)
zephyr_include_directories(include)
zephyr_include_directories(source)
