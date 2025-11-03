idf_component_register(
    SRCS 
        "main.c"
        "wifi_setup.c"
        "web_server.c"
    INCLUDE_DIRS "."
    REQUIRES 
        led_strip 
        dht 
        esp_wifi 
        esp_http_server
    PRIV_REQUIRES 
        nvs_flash 
        driver
)
