#include <WiFi.h>
#include "esp32/ulp.h"
#include "ulp_main.h"
#include "ulptool.h"

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* serverUrl = "http://yourserver.com/data";

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

#define DATA_SIZE 1000

void setup() {
    Serial.begin(115200);
    delay(1000);

    ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ulp_load_program(0, (uint32_t *)entry_start, sizeof(entry_start) / sizeof(uint32_t));
    ulp_run((&entry_start - RTC_SLOW_MEM) / sizeof(uint32_t));

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to Wi-Fi");
}

void loop() {
    uint32_t data[DATA_SIZE];
    for (int i = 0; i < 5; i++) {
        ulp_run((&entry_start - RTC_SLOW_MEM) / sizeof(uint32_t));
        ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
        ulp_set_wakeup_period(0, 1000 * 1000);

        Serial.println("Main CPU going to sleep...");
        esp_deep_sleep_start();
        
        ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));

        for (int j = 0; j < DATA_SIZE; j++) {
            ulp_read_microphone_data(j, &data[j]);
        }

        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(serverUrl);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            String postData = "data=";
            for (int j = 0; j < DATA_SIZE; j++) {
                postData += String(data[j]) + ",";
            }
            postData.remove(postData.length() - 1);
            int httpResponseCode = http.POST(postData);
            if (httpResponseCode > 0) {
                String response = http.getString();
                Serial.println("Response: " + response);
            } else {
                Serial.println("Error: " + String(httpResponseCode));
            }
            http.end();
        } else {
            Serial.println("Wi-Fi not connected");
        }

        delay(10000);
    }

    Serial.println("Main CPU going to sleep...");
    esp_deep_sleep_start();
}
