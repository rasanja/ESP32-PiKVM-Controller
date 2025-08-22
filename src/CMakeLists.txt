# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

idf_component_register(
        SRCS
        "main.c"
        "wifi.c"
        "pikvm.c"
        "control.c"
        INCLUDE_DIRS
        "."
        REQUIRES
        esp_http_client
        json            # cJSON
        esp_wifi
        esp_event
        esp_netif
        driver
        nvs_flash
)