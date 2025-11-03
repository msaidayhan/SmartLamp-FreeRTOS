#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <string.h>

extern float latest_temp;
extern float latest_hum;
extern QueueHandle_t led_cmd_queue;

static const char *TAG = "WEB_SERVER";

typedef enum {
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_AUTO
} led_color_t;

typedef struct {
    led_color_t color;
} led_command_t;

static led_color_t current_color = LED_COLOR_AUTO;

// ---------------------------------------------
// Ana HTML sayfası
// ---------------------------------------------
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char *html =
        "<!DOCTYPE html><html lang='tr'><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>ESP32 SmartLamp</title>"
        "<style>"
        "body{font-family:Arial;text-align:center;background:#0d1117;color:#fff;transition:background 1s ease;}"
        ".card{margin:auto;margin-top:40px;padding:25px;width:340px;background:#161b22;"
        "border-radius:20px;box-shadow:0 0 15px #00bfff,0 0 30px #0044ff;transition:box-shadow 1s ease;}"
        ".value{font-size:22px;margin:10px 0;}"
        ".bar{height:22px;border-radius:10px;overflow:hidden;background:#222;margin:15px;}"
        ".bar-fill{height:100%;width:0%;background:linear-gradient(90deg,#00f,#0f0,#ff0,#f00);"
        "transition:width 0.8s ease-in-out,background 1s ease;}"
        "button{margin:5px;padding:10px 20px;border:none;border-radius:8px;font-size:16px;"
        "color:#fff;cursor:pointer;opacity:0.8;transition:all 0.3s ease;}"
        "button:hover{transform:scale(1.05);opacity:1;}"
        "button.active{opacity:1;box-shadow:0 0 20px 4px currentColor;}"
        "button.red{background:#f00;}button.green{background:#0f0;color:#000;}"
        "button.blue{background:#00f;}button.auto{background:#aaa;color:#000;}"
        ".led-ring{width:100px;height:100px;border-radius:50%;margin:20px auto;"
        "background:radial-gradient(circle at center,#444 40%,#000 70%);"
        "box-shadow:0 0 20px #000 inset;transition:background 0.5s,box-shadow 0.5s;}"
        ".footer{margin-top:15px;font-size:12px;color:#888;}"
        "</style>"
        "<script>"
        "function updateTheme(color,hum){"
        "let bg=document.body.style;"
        "if(hum<55)bg.background='#0d1b3d';else if(hum<72)bg.background='#0d3d1b';else bg.background='#3d0d0d';"
        "let ring=document.getElementById('ring');"
        "ring.style.boxShadow='0 0 30px '+color+',0 0 60px '+color;"
        "ring.style.background='radial-gradient(circle at center,'+color+' 40%,#000 70%)';"
        "document.querySelector('.card').style.boxShadow='0 0 15px '+color+',0 0 40px '+color;"
        "}"
        "async function refresh(){"
        "let r=await fetch('/api/data');"
        "let d=await r.json();"
        "let hum=Math.min(Math.max(d.hum,0),100);"  // Nem %0–100 aralığına sabitleniyor
        "document.getElementById('temp').innerText=d.temp.toFixed(1)+' °C';"
        "document.getElementById('hum').innerText=hum.toFixed(1)+' %';"
        "let bar=document.getElementById('bar');"
        "bar.style.width=hum+'%';"
        // Renk geçişi: düşük nem → mavi, normal → yeşil, yüksek → kırmızı
        "if(hum<55)bar.style.background='linear-gradient(90deg,#00bfff,#0077ff)';"
        "else if(hum<72)bar.style.background='linear-gradient(90deg,#00ff80,#00ff00)';"
        "else bar.style.background='linear-gradient(90deg,#ff6600,#ff0000)';"
        "['red','green','blue','auto'].forEach(c=>{"
        "let b=document.getElementById(c);b.classList.remove('active');"
        "if(c===d.color)b.classList.add('active');});"
        "let col=d.color==='red'?'#f00':d.color==='green'?'#0f0':d.color==='blue'?'#00f':'#fff';"
        "updateTheme(col,hum);"
        "}"
        "setInterval(refresh,2000);"
        "function sendColor(c){fetch('/api/led?color='+c).then(()=>refresh());}"
        "</script>"
        "</head><body onload='refresh()'>"
        "<div class='card'>"
        "<h2>🌈 ESP32 SmartLamp</h2>"
        "<div class='led-ring' id='ring'></div>"
        "<div class='value'>🌡️ <span id='temp'>--</span></div>"
        "<div class='value'>💧 <span id='hum'>--</span></div>"
        "<div class='bar'><div class='bar-fill' id='bar'></div></div>"
        "<button id='red' class='red' onclick=\"sendColor('red')\">🔴</button>"
        "<button id='green' class='green' onclick=\"sendColor('green')\">🟢</button>"
        "<button id='blue' class='blue' onclick=\"sendColor('blue')\">🔵</button>"
        "<button id='auto' class='auto' onclick=\"sendColor('auto')\">⚪</button>"
        "<div class='footer'>Mustafa Said Dayhan 22370031086</div>"
        "</div></body></html>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

// ---------------------------------------------
// /api/data – JSON sensör verisi + mod
// ---------------------------------------------
static esp_err_t data_get_handler(httpd_req_t *req)
{
    char json[128];
    const char *color_str = 
        (current_color == LED_COLOR_RED)   ? "red" :
        (current_color == LED_COLOR_GREEN) ? "green" :
        (current_color == LED_COLOR_BLUE)  ? "blue" : "auto";

    snprintf(json, sizeof(json),
             "{\"temp\":%.1f,\"hum\":%.1f,\"color\":\"%s\"}",
             latest_temp, latest_hum, color_str);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

// ---------------------------------------------
// /api/led – Renk veya mod değiştirme
// ---------------------------------------------
static esp_err_t led_get_handler(httpd_req_t *req)
{
    char buf[32];
    size_t len = httpd_req_get_url_query_len(req) + 1;

    if (len > sizeof(buf)) len = sizeof(buf);
    if (httpd_req_get_url_query_str(req, buf, len) == ESP_OK) {
        char color[16];
        if (httpd_query_key_value(buf, "color", color, sizeof(color)) == ESP_OK) {
            led_command_t cmd;

            if (strcmp(color, "red") == 0)
                cmd.color = LED_COLOR_RED;
            else if (strcmp(color, "green") == 0)
                cmd.color = LED_COLOR_GREEN;
            else if (strcmp(color, "blue") == 0)
                cmd.color = LED_COLOR_BLUE;
            else
                cmd.color = LED_COLOR_AUTO;

            current_color = cmd.color;
            xQueueSend(led_cmd_queue, &cmd, 0);
            ESP_LOGI(TAG, "Mode changed to: %s", color);
        }
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
}

// ---------------------------------------------
// Web sunucusunu başlat
// ---------------------------------------------
void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = { .uri="/", .method=HTTP_GET, .handler=root_get_handler };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t data = { .uri="/api/data", .method=HTTP_GET, .handler=data_get_handler };
        httpd_register_uri_handler(server, &data);

        httpd_uri_t led = { .uri="/api/led", .method=HTTP_GET, .handler=led_get_handler };
        httpd_register_uri_handler(server, &led);

        ESP_LOGI(TAG, "Web sunucusu başlatıldı.");
    } else {
        ESP_LOGE(TAG, "Web sunucusu başlatılamadı!");
    }
}
