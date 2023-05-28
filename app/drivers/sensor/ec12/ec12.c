#define DT_DRV_COMPAT alps_ec12

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "ec12.h"

int ec12_init(const struct device *dev) {
    return 0;
}

#define EC12_INST(n)                                                                               \
    const struct ec12_config ec12_cfg_##n = {                                                      \
        .a = GPIO_DT_SPEC_INST_GET(n, a_gpios),                                                    \
        .b = GPIO_DT_SPEC_INST_GET(n, b_gpios),                                                    \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, resolution), (1), (DT_INST_PROP(n, resolution))),     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, ec12_init, NULL, NULL, &ec12_cfg_##n, POST_KERNEL,          \
                          CONFIG_SENSOR_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(EC12_INST)