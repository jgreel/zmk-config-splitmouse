#include <drivers/sensor.h>
#include <logging/log.h>

#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <dt-bindings/zmk/mouse.h>

#define SCROLL_DIV_FACTOR 5

LOG_MODULE_REGISTER(trackpad, CONFIG_SENSOR_LOG_LEVEL);

const struct device *trackpad = DEVICE_DT_GET(DT_INST(0, cirque_pinnacle));

static void handle_trackpad(const struct device *dev, const struct sensor_trigger *trig) {
    static uint8_t last_pressed = 0;
    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        LOG_ERR("fetch: %d", ret);
        return;
    }
    struct sensor_value dx, dy, btn;
    ret = sensor_channel_get(dev, SENSOR_CHAN_POS_DX, &dx);
    if (ret < 0) {
        LOG_ERR("get dx: %d", ret);
        return;
    }
    ret = sensor_channel_get(dev, SENSOR_CHAN_POS_DY, &dy);
    if (ret < 0) {
        LOG_ERR("get dy: %d", ret);
        return;
    }
    ret = sensor_channel_get(dev, SENSOR_CHAN_PRESS, &btn);
    if (ret < 0) {
        LOG_ERR("get btn: %d", ret);
        return;
    }
    LOG_DBG("trackpad %d %d %02x", dx.val1, dy.val1, btn.val1);
    zmk_hid_mouse_movement_set(0, 0);
    zmk_hid_mouse_scroll_set(0, 0);
    const uint8_t layer = zmk_keymap_highest_layer_active();
    uint8_t button;
    static uint8_t last_button = 0;
    static int8_t scroll_ver_rem = 0, scroll_hor_rem = 0;
    if (layer == 1) {   // raise
        const int16_t total_hor = dx.val1 + scroll_hor_rem, total_ver = -dy.val1 + scroll_ver_rem;
        scroll_hor_rem = total_hor % SCROLL_DIV_FACTOR;
        scroll_ver_rem = total_ver % SCROLL_DIV_FACTOR;
        zmk_hid_mouse_scroll_update(total_hor / SCROLL_DIV_FACTOR, total_ver / SCROLL_DIV_FACTOR);
        button = RCLK;
    } else {
        zmk_hid_mouse_movement_update(CLAMP(-dx.val1, INT8_MIN, INT8_MAX), CLAMP(dy.val1, INT8_MIN, INT8_MAX));
        button = LCLK;
    }
    if (!last_pressed && btn.val1) {
        zmk_hid_mouse_buttons_press(button);
        last_button = button;
    } else if (last_pressed && !btn.val1) {
        zmk_hid_mouse_buttons_release(last_button);
    }
    zmk_endpoints_send_mouse_report();
    last_pressed = btn.val1;
}

static int trackpad_init() {
    struct sensor_trigger trigger = {
        .type = SENSOR_TRIG_DATA_READY,
        .chan = SENSOR_CHAN_ALL,
    };
    printk("trackpad");
    if (sensor_trigger_set(trackpad, &trigger, handle_trackpad) < 0) {
        LOG_ERR("can't set trigger");
        return -EIO;
    };
    return 0;
}

SYS_INIT(trackpad_init, APPLICATION, CONFIG_ZMK_KSCAN_INIT_PRIORITY);