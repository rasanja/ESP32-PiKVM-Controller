//
// Created by Rasanja Dampriya on 8/21/25.
//

#include "pikvm.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "PIKVM";

// Event handler: print any response body
static esp_err_t http_evt(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0) {
        fwrite(evt->data, 1, evt->data_len, stdout);
    }
    return ESP_OK;
}

// Core helper: POST /api/atx/power?action=on|off
static esp_err_t pikvm_post_power(const char *ip, const char *user, const char *pass, const char *action) {
    if (!ip || !user || !pass || !action) return ESP_ERR_INVALID_ARG;

    char url[160];
    snprintf(url, sizeof(url), "http://%s/api/atx/power?action=%s", ip, action);

    esp_http_client_config_t cfg = {
        .url = url,
        .event_handler = http_evt,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .username = user,
        .password = pass,
        .timeout_ms = 5000,
        .cert_pem = NULL,
        .skip_cert_common_name_check = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "http client init failed");
        return ESP_FAIL;
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, "", 0); // empty body

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP %d for %s", status, url);

    esp_http_client_cleanup(client);
    return (status >= 200 && status < 300) ? ESP_OK : ESP_FAIL;
}

esp_err_t pikvm_power_on(const char *ip, const char *user, const char *pass) {
    return pikvm_post_power(ip, user, pass, "on");
}

esp_err_t pikvm_power_off(const char *ip, const char *user, const char *pass) {
    return pikvm_post_power(ip, user, pass, "off");
}