#include <string>
#include <cstring>
#include <cstdint>
#include "main.hpp"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "Arduino.h"
#include "DHT.h"

//set with make menuconfig
#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD

#define DHTPIN 4        // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

extern "C" void app_main(void)
{
    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_sta_config_t sta_config;
    uint8_t ssid[32] = DEFAULT_SSID;
    memcpy(sta_config.ssid, ssid, sizeof(ssid));
    uint8_t password[64] = DEFAULT_PWD;
    memcpy(sta_config.password, password, sizeof(password));
    sta_config.bssid_set = false;
    wifi_config_t wifi_config;
    wifi_config.sta = sta_config;
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

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
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

