#pragma once
#include "esp_err.h"
#include <stdbool.h>

// Power controls
esp_err_t pikvm_power_on(const char *ip, const char *user, const char *pass);
esp_err_t pikvm_power_off(const char *ip, const char *user, const char *pass);
esp_err_t pikvm_get_atx_state(const char *host, const char *user, const char *pass, bool *is_on);