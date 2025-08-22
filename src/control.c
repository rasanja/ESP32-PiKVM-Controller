//
// Created by Rasanja Dampriya on 8/22/25.
//

#include "control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "pikvm.h"

// Pin mapping
#define PIN_LED     GPIO_NUM_13   // D13, LED active HIGH
#define PIN_SWITCH  GPIO_NUM_14   // D14, toggle, active LOW with pull-up

// Timing
#define DEBOUNCE_MS        50
#define POLL_INTERVAL_MS   500
#define CONFIRM_RETRIES    40     // ~20 s

static const char *TAG = "CTRL";

static inline void led_set(bool on) {
    gpio_set_level(PIN_LED, on ? 1 : 0);
}

static inline bool switch_is_on(void) {
    // Active LOW wiring: pulled to GND when "ON"
    return gpio_get_level(PIN_SWITCH) == 0;
}

static bool poll_atx_until(bool want_on) {
    bool is_on = false;
    for (int i = 0; i < CONFIRM_RETRIES; i++) {
        if (pikvm_get_atx_state(CONFIG_PIKVM_HOST, CONFIG_PIKVM_USER, CONFIG_PIKVM_PASS, &is_on) == ESP_OK) {
            if (is_on == want_on) return true;
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
    return false;
}

static void control_task(void *arg) {
    // GPIO init
    gpio_config_t out = {
        .pin_bit_mask = 1ULL << PIN_LED,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out);

    gpio_config_t in = {
        .pin_bit_mask = 1ULL << PIN_SWITCH,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&in);

    // Boot: read ATX state and reflect on LED
    bool atx_on = false;
    if (pikvm_get_atx_state(CONFIG_PIKVM_HOST, CONFIG_PIKVM_USER, CONFIG_PIKVM_PASS, &atx_on) == ESP_OK) {
        ESP_LOGI(TAG, "Boot ATX state: %s", atx_on ? "ON" : "OFF");
        led_set(atx_on);
    } else {
        ESP_LOGW(TAG, "ATX state read failed at boot; LED off");
        led_set(false);
    }

    // Track debounced switch state
    bool last = switch_is_on();

    while (1) {
        // debounce
        bool s1 = switch_is_on();
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        bool s2 = switch_is_on();

        if (s1 == s2 && s2 != last) {
            last = s2;

            if (last) {
                // Switch flipped ON -> request ON, confirm, LED on
                ESP_LOGI(TAG, "Switch ON: sending power ON");
                if (pikvm_power_on(CONFIG_PIKVM_HOST, CONFIG_PIKVM_USER, CONFIG_PIKVM_PASS) == ESP_OK) {
                    if (poll_atx_until(true)) {
                        ESP_LOGI(TAG, "Confirmed ON");
                        led_set(true);
                    } else {
                        ESP_LOGW(TAG, "ON not confirmed within timeout");
                    }
                } else {
                    ESP_LOGE(TAG, "power ON request failed");
                }
            } else {
                // Switch flipped OFF -> request OFF, confirm, LED off
                ESP_LOGI(TAG, "Switch OFF: sending power OFF");
                if (pikvm_power_off(CONFIG_PIKVM_HOST, CONFIG_PIKVM_USER, CONFIG_PIKVM_PASS) == ESP_OK) {
                    if (poll_atx_until(false)) {
                        ESP_LOGI(TAG, "Confirmed OFF");
                        led_set(false);
                    } else {
                        ESP_LOGW(TAG, "OFF not confirmed within timeout");
                    }
                } else {
                    ESP_LOGE(TAG, "power OFF request failed");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void start_control_task(void) {
    xTaskCreate(control_task, "control_task", 4096, NULL, 5, NULL);
}
