#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "led_strip.h"
#include "dht.h"
#include "wifi_setup.h"
#include "web_server.h"

#define DHT_GPIO 4
#define LED_GPIO 18
#define LED_COUNT 12

static const char *TAG = "SMARTLAMP";

led_strip_handle_t strip;
QueueHandle_t led_cmd_queue;

float latest_temp = 0.0f;
float latest_hum = 0.0f;
bool auto_mode = true;  // başlangıçta otomatik mod

typedef enum {
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_AUTO
} led_color_t;

typedef struct {
    led_color_t color;
} led_command_t;

uint8_t current_r = 0, current_g = 0, current_b = 0;

// --------------------------------------------------
// Renk geçişi (smooth fade)
// --------------------------------------------------
void smooth_transition(uint8_t target_r, uint8_t target_g, uint8_t target_b, int steps, int delay_ms) {
    float dr = ((float)target_r - current_r) / steps;
    float dg = ((float)target_g - current_g) / steps;
    float db = ((float)target_b - current_b) / steps;

    for (int i = 0; i < steps; i++) {
        uint8_t r = current_r + dr * i;
        uint8_t g = current_g + dg * i;
        uint8_t b = current_b + db * i;

        for (int j = 0; j < LED_COUNT; j++) {
            led_strip_set_pixel(strip, j, r, g, b);
        }
        led_strip_refresh(strip);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    current_r = target_r;
    current_g = target_g;
    current_b = target_b;
}

// --------------------------------------------------
// Neme göre LED rengi
// --------------------------------------------------
void update_led_by_humidity(float humidity) {
    uint8_t r, g, b;

    if (humidity < 55.0) {
        r = 0; g = 100; b = 255;  // düşük nem = mavi
    } else if (humidity < 72.0) {
        r = 0; g = 255; b = 80;   // normal = yeşil
    } else {
        r = 255; g = 60; b = 0;   // yüksek = kırmızı
    }

    smooth_transition(r, g, b, 20, 25);
}

// --------------------------------------------------
// LED Task – Queue’dan gelen komutları dinler
// --------------------------------------------------
void led_task(void *pvParameters) {
    led_command_t cmd;

    while (1) {
        if (xQueueReceive(led_cmd_queue, &cmd, pdMS_TO_TICKS(50))) {
            switch (cmd.color) {
                case LED_COLOR_RED:
                    auto_mode = false;
                    smooth_transition(255, 0, 0, 20, 25);
                    break;
                case LED_COLOR_GREEN:
                    auto_mode = false;
                    smooth_transition(0, 255, 0, 20, 25);
                    break;
                case LED_COLOR_BLUE:
                    auto_mode = false;
                    smooth_transition(0, 0, 255, 20, 25);
                    break;
                case LED_COLOR_AUTO:
                    auto_mode = true;
                    ESP_LOGI(TAG, "Auto mode enabled");
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --------------------------------------------------
// DHT sensör task
// --------------------------------------------------
void dht_task(void *pvParameters) {
    float temperature, humidity;

    while (1) {
        if (dht_read_float_data(DHT_TYPE_AM2301, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
            latest_temp = temperature;
            latest_hum = humidity;

            ESP_LOGI(TAG, "Temp: %.1f°C | Hum: %.1f%%", temperature, humidity);

            if (auto_mode) {
                update_led_by_humidity(humidity);
            }
        } else {
            ESP_LOGW(TAG, "DHT read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

// --------------------------------------------------
// Başlatma
// --------------------------------------------------
void app_main() {
    ESP_LOGI(TAG, "Starting SmartLamp (Queue + Auto Mode)");

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 5 * 1000 * 1000,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));
    led_strip_clear(strip);

    led_cmd_queue = xQueueCreate(5, sizeof(led_command_t));

    wifi_init_sta();
    start_webserver();

    xTaskCreatePinnedToCore(dht_task, "dht_task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(led_task, "led_task", 4096, NULL, 5, NULL, 1);
}
