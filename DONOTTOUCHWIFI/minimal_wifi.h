#ifndef MINIMAL_WIFI_H
#define MINIMAL_WIFI_H

void wifi_connect(const char* ssid, const char* pass);
esp_mqtt_client_handle_t wifi_setup(const char *ssid, const char *pass, const char *broker);

#endif
