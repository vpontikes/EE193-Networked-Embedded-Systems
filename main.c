#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <soc/gpio_reg.h>
#include <soc/gpio_struct.h>
//Temperature IC
#include "driver/spi_master.h"
#include <string.h>
//Thermistor
#include "esp_adc/adc_oneshot.h"
#include <math.h>
//Wifi
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "minimal_wifi.h"
//Initial Pin Definitions
#define CS_GPIO 5
#define MOSI_GPIO 4
#define SCLK_GPIO 18
#define ADC_CHANNEL ADC_CHANNEL_1
//Wifi Setup
#define WIFI_SSID      "Tufts_Wireless"
#define WIFI_PASS      ""
#define BROKER_URI "mqtt://en1-pi.eecs.tufts.edu"
void app_main(){
    // Configure the ADC ----------------------------------------------------------------
    adc_oneshot_unit_init_cfg_t adc1_init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_new_unit(&adc1_init_cfg, &adc1_handle);
    // Configure the channel within the ADC
    adc_oneshot_chan_cfg_t adc1_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Default is 12 bits (max)
        .atten = ADC_ATTEN_DB_11
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &adc1_cfg);
    //Temperature IC Setup ----------------------------------------------------------------
    esp_err_t ret;
    spi_device_handle_t spi;
    //Setup bus
    spi_bus_config_t buscfg={
        .miso_io_num = -1,
        .mosi_io_num = MOSI_GPIO,
        .sclk_io_num = SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    //Setup TMP126
    spi_device_interface_config_t devcfg={
        .clock_speed_hz = 1000*1000,
        .mode = 0,
        .spics_io_num = CS_GPIO,
        .flags = SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX,
        .queue_size = 1,
        .command_bits = 16
    };
    //Initialize SPI bus
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    //Temperature IC Read
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 0;
    t.rxlength = 16;
    t.cmd = (1<<8);
    t.tx_buffer = NULL;
    uint8_t rx_buffer[2] = {0, 0};
    t.rx_buffer = &rx_buffer;
    ret = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
    uint16_t rx_buffer_real = rx_buffer[0]*256 + rx_buffer[1];
    rx_buffer_real = (rx_buffer_real >> 2);
    //Save it in a register
    //...
    //Read Battery
    float adc_raw;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw);
    float battery = adc_raw*2; //Placeholder expression
    //Wifi Setup ----------------------------------------------------------------
    esp_err_t ret2 = nvs_flash_init();
    if (ret2 == ESP_ERR_NVS_NO_FREE_PAGES || ret2 == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret2 = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret2);
    //printf("Connecting to WiFi...");
    wifi_connect(WIFI_SSID, WIFI_PASS);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    //int msg_id = esp_mqtt_client_subscribe(client, "/time/qos1", 1);
    //Subscribe to Time Topic
    int timestamp = esp_mqtt_client_subscribe(client, "time", 1);
    //Construct Message
    //format: timestamp,temperature,battery
    char message[23];
    sprintf(message, "%d,%f,%f", timestamp, 0.03125*rx_buffer_real, battery);
    //Publish to MQTT
    printf("Sending MQTT message in 5 seconds...");
    esp_mqtt_client_publish(client, "teamI/node1/tempupdate", message, 23, 0, 1);
    //Enter Deep Sleep (for x seconds)
    esp_sleep_enable_timer_wakeup(1800000000);
    esp_deep_sleep_start();
}
