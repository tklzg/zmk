/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>

#include <zmk/activity.h>

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/usb.h>
#endif

bool is_usb_power_present() {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    return zmk_usb_is_powered();
#else
    return false;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
}

struct ec12_config {
    const struct gpio_dt_spec a;
    const struct gpio_dt_spec b;

    const uint8_t resolution;
};

static const struct device *left_encoder = DEVICE_DT_GET(DT_CHOSEN(zmk_left_encoder_3));
static const struct device *left_encoder_2 = DEVICE_DT_GET(DT_CHOSEN(zmk_left_encoder_4));
static const struct device *right_encoder = DEVICE_DT_GET(DT_CHOSEN(zmk_right_encoder_3));
static const struct device *right_encoder_2 = DEVICE_DT_GET(DT_CHOSEN(zmk_right_encoder_4));

static bool flag_for_encoder_disable = false;
static bool flag_for_force_encoder_disable = false;

static enum zmk_activity_state activity_state;

static uint32_t activity_last_uptime;

#define MAX_IDLE_MS CONFIG_ZMK_IDLE_TIMEOUT

#if IS_ENABLED(CONFIG_ZMK_SLEEP)
#define MAX_SLEEP_MS CONFIG_ZMK_IDLE_SLEEP_TIMEOUT
#endif

int raise_event() {
    return ZMK_EVENT_RAISE(new_zmk_activity_state_changed(
        (struct zmk_activity_state_changed){.state = activity_state}));
}

int set_state(enum zmk_activity_state state) {
    if (activity_state == state)
        return 0;

    activity_state = state;
    return raise_event();
}

enum zmk_activity_state zmk_activity_get_state() { return activity_state; }

int activity_event_listener(const zmk_event_t *eh) {
    activity_last_uptime = k_uptime_get();

    return set_state(ZMK_ACTIVITY_ACTIVE);
}

void disable_encoder(struct ec12_config *cfg)
{
    gpio_pin_interrupt_configure_dt(&cfg->a, GPIO_INT_DISABLE);
    gpio_pin_configure_dt(&cfg->a, GPIO_OPEN_DRAIN);

    gpio_pin_interrupt_configure_dt(&cfg->b, GPIO_INT_DISABLE);
    gpio_pin_configure_dt(&cfg->b, GPIO_OPEN_DRAIN);
}

void enable_encoder(struct ec12_config *cfg)
{
    gpio_pin_configure_dt(&cfg->a, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&cfg->a, GPIO_INT_EDGE_BOTH);

    gpio_pin_configure_dt(&cfg->b, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&cfg->b, GPIO_INT_EDGE_BOTH);
}

void disable_encoder_all()
{
    LOG_DBG("disable encoder");
    disable_encoder(left_encoder->config);
    disable_encoder(left_encoder_2->config);
    disable_encoder(right_encoder->config);
    disable_encoder(right_encoder_2->config);
}

void disable_encoder_all_force()
{
    LOG_DBG("force disable encoder");
    flag_for_force_encoder_disable = true;
    disable_encoder(left_encoder->config);
    disable_encoder(left_encoder_2->config);
    disable_encoder(right_encoder->config);
    disable_encoder(right_encoder_2->config);
}

void enable_encoder_all()
{
    LOG_DBG("enable encoder");
    flag_for_force_encoder_disable = false;
    enable_encoder(left_encoder->config);
    enable_encoder(left_encoder_2->config);
    enable_encoder(right_encoder->config);
    enable_encoder(right_encoder_2->config);
}

void activity_work_handler(struct k_work *work) {
    int32_t current = k_uptime_get();
    int32_t inactive_time = current - activity_last_uptime;
#if IS_ENABLED(CONFIG_ZMK_SLEEP)
    if (inactive_time > MAX_SLEEP_MS && !is_usb_power_present()) {
        // Put devices in suspend power mode before sleeping
        if(flag_for_encoder_disable == false)
        {
            disable_encoder_all();

            flag_for_encoder_disable = true;
        }

        set_state(ZMK_ACTIVITY_SLEEP);

        pm_state_force(0U, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
    } else
#endif /* IS_ENABLED(CONFIG_ZMK_SLEEP) */
        if (inactive_time > MAX_IDLE_MS) {

            if(flag_for_encoder_disable == false)
            {
                disable_encoder_all();

                flag_for_encoder_disable = true;
            }

            set_state(ZMK_ACTIVITY_IDLE);
        }
        else if(flag_for_encoder_disable == true && flag_for_force_encoder_disable == false)
        {
            enable_encoder_all();

            flag_for_encoder_disable = false;
        }
}


K_WORK_DEFINE(activity_work, activity_work_handler);

void activity_expiry_function() { k_work_submit(&activity_work); }

K_TIMER_DEFINE(activity_timer, activity_expiry_function, NULL);

int activity_init() {
    activity_last_uptime = k_uptime_get();

    k_timer_start(&activity_timer, K_SECONDS(1), K_SECONDS(1));
    return 0;
}

ZMK_LISTENER(activity, activity_event_listener);
ZMK_SUBSCRIPTION(activity, zmk_position_state_changed);
ZMK_SUBSCRIPTION(activity, zmk_sensor_event);

SYS_INIT(activity_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);