#include "driver/adc.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>
#include <string.h>

#define SAMPLE_SIZE 1000
#define MAX_SAMPLES 3000
#define WIFI_SSID "QViet"
#define WIFI_PASSWORD "ThuyenThu76@"

#define BUZZER_PIN 17
#define GAIN_THRESHOLD 100
#define DEEP_SLEEP_TIME 3000

uint16_t cpu_samples[MAX_SAMPLES];
unsigned long timestamps[MAX_SAMPLES];
size_t total_samples = 0;
uint16_t buzzer_flag = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ADC Sample Collection");

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_8, ADC_ATTEN_DB_11);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    while (true) {
        Serial.println("Reading ADC samples...");
        for (int i = 0; i < SAMPLE_SIZE; i++) {
            int raw_value = adc1_get_raw(ADC1_CHANNEL_8);
            if (raw_value > 500 || raw_value > 64165.0) {
                Serial.printf("Skipped raw value: %d\n", raw_value);
                continue;
            }
            
            int processed_data = log(raw_value / 50.0) * 300 + 20;

            if (total_samples < MAX_SAMPLES) {
                cpu_samples[total_samples] = processed_data;

                if (total_samples % 50 == 0) {
                    timestamps[total_samples] = millis();
                }
                total_samples++;

                Serial.printf("Measurement %d: %4i at %lu ms\n", total_samples, cpu_samples[total_samples - 1], timestamps[total_samples - 1]);

                if (processed_data > GAIN_THRESHOLD && processed_data < 64165.0) {
                    buzzer_flag += 1;
                }
                if (buzzer_flag == 1) {
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(1500);
                    digitalWrite(BUZZER_PIN, LOW);
                    buzzer_flag = 0;
                }
            }
            delay(10);
        }
        Serial.printf("\nCollected %d samples.\n", total_samples);

        if (total_samples >= MAX_SAMPLES) {
            String jsonData = "[";
            for (size_t i = 0; i < total_samples; i += 50) {
                if (i + 50 <= total_samples) {
                    float sum = 0;
                    int count = 0;

                    for (size_t j = 0; j < 50; j++) {
                        int currentSample = cpu_samples[i + j];
                        if (currentSample <= 2000 && currentSample >= 10) {
                            sum += currentSample;
                            count++;
                        }
                    }

                    float average = (count > 0) ? (sum / count) : 0;

                    if (average < 20) {
                        average = 20;
                    } else if (average > 90) {
                        average = 75;
                    }

                    jsonData += "{\"timestamp\":" + String(timestamps[i]) + ",\"average\":" + String(average) + "}";
                    if (i + 50 < total_samples) {
                        jsonData += ",";
                    }
                }
            }
            jsonData += "]";

            Serial.println("Sending the following samples:");
            Serial.println(jsonData);

            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            Serial.print("Connecting to WiFi...");
            int connectionAttempts = 0;
            while (WiFi.status() != WL_CONNECTED && connectionAttempts < 10) {
                delay(1000);
                Serial.println("Connecting...");
                connectionAttempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi!");

                HTTPClient http;
                http.begin("http://192.168.50.230:3000/data");
                http.addHeader("Content-Type", "application/json");
                int httpResponseCode = http.POST(jsonData);

                if (httpResponseCode > 0) {
                    Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
                    String responseMessage = http.getString();
                    Serial.println("Response from server:");
                    Serial.println(responseMessage);
                } else {
                    Serial.printf("Error on sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
                }

                http.end();
            } else {
                Serial.println("Failed to connect to WiFi.");
            }
            WiFi.disconnect();
            Serial.println("Disconnected from WiFi");

            total_samples = 0;
            memset(cpu_samples, 0, sizeof(cpu_samples));
            memset(timestamps, 0, sizeof(timestamps));
        }
    }
}

void loop() {
}
