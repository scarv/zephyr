# Copyright (c) 2019 SiFive Inc.
# SPDX-License-Identifier: Apache-2.0
zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_compile_options(-march=rv32imc_xcrypto)
