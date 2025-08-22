#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "pikvm.h"
#include "wifi.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "control.h"

#define PIN_LED     GPIO_NUM_13    // D13
#define PIN_SWITCH  GPIO_NUM_14    // D14, active LOW

static const char *TAG = "APP";

static bool read_switch_active_low(void) {
	// returns true when switch is toggled OFF (pulled to GND)
	return gpio_get_level(PIN_SWITCH) == 0;
}

void app_main(void)
{

	//initialize storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

	ESP_ERROR_CHECK(wifi_init_and_connect());

	//start PiKVM Controller
	start_control_task();

	// Use values from sdkconfig (the “environment file”)
	//ESP_LOGI(TAG, "Powering ON via PiKVM at %s", CONFIG_PIKVM_HOST);
	//ESP_ERROR_CHECK(pikvm_power_on(CONFIG_PIKVM_HOST, CONFIG_PIKVM_USER, CONFIG_PIKVM_PASS));

	// Example OFF later
	// ESP_ERROR_CHECK(pikvm_power_off(CONFIG_PIKVM_HOST, CONFIG_PIKVM_USER, CONFIG_PIKVM_PASS));
}