/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

struct ec12_config {
    const struct gpio_dt_spec a;
    const struct gpio_dt_spec b;

    const uint8_t resolution;
};

