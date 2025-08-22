#pragma once
#include "esp_err.h"

// Power controls
esp_err_t pikvm_power_on(const char *ip, const char *user, const char *pass);
esp_err_t pikvm_power_off(const char *ip, const char *user, const char *pass);