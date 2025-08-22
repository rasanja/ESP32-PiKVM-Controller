//
// Created by Rasanja Dampriya on 8/21/25.
//

#include "pikvm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

static const char *TAG = "PIKVM";

/* ---------- Response buffer helper for chunked HTTP ---------- */
typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
} resp_buf_t;

static void rb_init(resp_buf_t *rb, size_t cap) {
    rb->buf = (char *)malloc(cap);
    rb->len = 0;
    rb->cap = rb->buf ? cap : 0;
    if (rb->buf) rb->buf[0] = '\0';
}

static void rb_free(resp_buf_t *rb) {
    free(rb->buf);
    rb->buf = NULL; rb->len = rb->cap = 0;
}

static void rb_append(resp_buf_t *rb, const char *data, size_t n) {
    if (!rb->buf || rb->cap - rb->len < n + 1) {
        size_t newcap = rb->cap ? rb->cap : 512;
        while (newcap - rb->len < n + 1) newcap *= 2;
        char *p = (char *)realloc(rb->buf, newcap);
        if (!p) return; // OOM; caller will fail parse later
        rb->buf = p; rb->cap = newcap;
    }
    memcpy(rb->buf + rb->len, data, n);
    rb->len += n;
    rb->buf[rb->len] = '\0';
}

/* Capture body chunks into resp_buf_t via user_data */
static esp_err_t http_evt_capture(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0) {
        resp_buf_t *rb = (resp_buf_t *)evt->user_data;
        if (rb) rb_append(rb, (const char *)evt->data, (size_t)evt->data_len);
    }
    return ESP_OK;
}

/* Optional: event handler that prints body (kept for POST debug) */
static esp_err_t http_evt_print(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0) {
        fwrite(evt->data, 1, evt->data_len, stdout);
    }
    return ESP_OK;
}

/* ---------- Core helper: POST /api/atx/power?action=on|off ---------- */
static esp_err_t pikvm_post_power(const char *host, const char *user, const char *pass, const char *action) {
    if (!host || !user || !pass || !action) return ESP_ERR_INVALID_ARG;

    char url[160];
    /* Use HTTPS directly to avoid 301 redirect from HTTP */
    snprintf(url, sizeof(url), "https://%s/api/atx/power?action=%s", host, action);

    esp_http_client_config_t cfg = {
        .url = url,
        .event_handler = http_evt_print,  // optional: prints tiny JSON like {"ok":true,"result":{}}
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .username = user,
        .password = pass,
        .timeout_ms = 7000,

        // Insecure like curl -k (you enabled this in menuconfig)
        .cert_pem = NULL,
        .skip_cert_common_name_check = true,

        // Buffers
        .buffer_size = 1024,
        .buffer_size_tx = 512,
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

esp_err_t pikvm_power_on(const char *host, const char *user, const char *pass) {
    return pikvm_post_power(host, user, pass, "on");
}

esp_err_t pikvm_power_off(const char *host, const char *user, const char *pass) {
    return pikvm_post_power(host, user, pass, "off");
}

/* ---------- GET /api/atx → parse result.leds.power ---------- */
esp_err_t pikvm_get_atx_state(const char *host, const char *user, const char *pass, bool *is_on) {
    if (!host || !user || !pass || !is_on) return ESP_ERR_INVALID_ARG;

    char url[192];
    snprintf(url, sizeof(url), "https://%s/api/atx", host);

    resp_buf_t rb;
    rb_init(&rb, 512);

    esp_http_client_config_t cfg = {
        .url = url,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .username = user,
        .password = pass,
        .timeout_ms = 7000,

        // Insecure like curl -k
        .cert_pem = NULL,
        .skip_cert_common_name_check = true,

        .event_handler = http_evt_capture,
        .user_data = &rb,
        .buffer_size = 1024,   // helps with chunked responses
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) { rb_free(&rb); return ESP_FAIL; }

    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);
    int status = (err == ESP_OK) ? esp_http_client_get_status_code(client) : -1;
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status < 200 || status >= 300 || rb.len == 0) {
        ESP_LOGE(TAG, "ATX GET failed: err=%s http=%d len=%u",
                 esp_err_to_name(err), status, (unsigned)rb.len);
        rb_free(&rb);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "ATX body: %s", rb.buf);

    // Example:
    // {"ok":true,"result":{"busy":false,"enabled":true,"leds":{"hdd":false,"power":false}}}
    cJSON *root = cJSON_Parse(rb.buf);
    rb_free(&rb);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error");
        return ESP_FAIL;
    }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    cJSON *leds   = result ? cJSON_GetObjectItem(result, "leds") : NULL;
    cJSON *power  = leds   ? cJSON_GetObjectItem(leds, "power")  : NULL;

    esp_err_t ret = ESP_FAIL;
    if (cJSON_IsBool(power)) {
        *is_on = cJSON_IsTrue(power);
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "Missing result.leds.power");
    }

    cJSON_Delete(root);
    return ret;
}