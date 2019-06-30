#include <string>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include "main.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "Arduino.h"
#include "DHT.h"
extern "C" {
    #include "esp_request.h"
}
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
static const char *TAG = "REQAPP";

//set with make menuconfig
#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD
#define LOCATION_TAG CONFIG_LOCATION_TAG

#define DHTPIN 4        // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}


static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_sta_config_t sta_config;
    uint8_t ssid[32] = DEFAULT_SSID;
    memcpy(sta_config.ssid, ssid, sizeof(ssid));
    uint8_t password[64] = DEFAULT_PWD;
    memcpy(sta_config.password, password, sizeof(password));
    sta_config.bssid_set = false;
    wifi_config_t wifi_config;
    wifi_config.sta = sta_config;
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    
}

extern "C" void app_main(void)
{
    nvs_flash_init();
    initialise_wifi();
    request_t *req;
    int status;
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 30000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Connected to AP, freemem=%d",esp_get_free_heap_size());

    char data[1024];

    dht.begin();
    while (true) {
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();
        if (isnan(humidity) || isnan(temperature)) {
            printf("Failed to read from DHT sensor!");
        }
        else {
            printf("Temperature %g °C\n", temperature);
            printf("humidity %g %%\n", humidity);
        }
        sprintf(data, "environment,location=%s temperature=%g,humidity=%g", LOCATION_TAG, temperature, humidity);
        request_t *req;
        int status;
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP, freemem=%d",esp_get_free_heap_size());
        // vTaskDelay(1000/portTICK_RATE_MS);
        req = req_new("http://rpi.odin:8086/write?db=temptest");
        char post[] = "POST";
        req_setopt(req, REQ_SET_METHOD, post);
        req_setopt(req, REQ_SET_POSTFIELDS, data);
        status = req_perform(req);
        req_clean(req);
        ESP_LOGI(TAG, "Finish request, status=%d, freemem=%d", status, esp_get_free_heap_size());

        esp_sleep_enable_timer_wakeup(1000*1000*60);
        esp_deep_sleep_start();
        //vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    
}

