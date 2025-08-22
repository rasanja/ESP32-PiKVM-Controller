//
// Created by Rasanja Dampriya on 8/21/25.
//

#pragma once
#include "esp_err.h"

// Connect STA using values from sdkconfig (menuconfig)
// Returns ESP_OK on success, else error code.
esp_err_t wifi_init_and_connect(void);