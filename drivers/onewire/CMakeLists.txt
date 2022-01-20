#
# Copyright (c) 2020 Martin Anschuetz
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

#zephyr_library_sources_ifdef(CONFIG_ONEWIRE onewire.c)

if (CONFIG_ONEWIRE_INTERFACE_SW)

    zephyr_library_sources_ifdef(CONFIG_ONEWIRE 
        sw-onewire-hal.c
    )

endif()
