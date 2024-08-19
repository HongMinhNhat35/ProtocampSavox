#include "driver/adc.h"
#include "esp32/ulp.h"
#include "soc/rtc_cntl_reg.h"
#include <string.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>

#define SAMPLE_SIZE 1000
#define MAX_SAMPLES 5000
#define WIFI_SSID "aalto open"
#define WIFI_PASSWORD ""
#define REF 1650.0
#define BUZZER_PIN 13
#define GAIN_THRESHOLD 80.0

uint16_t cpu_samples[MAX_SAMPLES];
size_t current_sample_count = 0;
size_t total_samples = 0;

const ulp_insn_t program[] = {
    I_MOVI  (R1, 0),
    I_MOVI  (R2, 0),

    M_LABEL (1),
        I_MOVI  (R0, 20),
        M_LABEL (2),
            I_DELAY (4000),
            I_SUBI  (R0, R0, 1),
        M_BGE (2, 1),

        I_ADC   (R3, 0, 6),
        I_ADDI  (R2, R2, 1),
        I_ST    (R3, R2, 100),
        I_ST    (R2, R1, 100),

    I_MOVR  (R0, R2),
    M_BL    (1, SAMPLE_SIZE),

    I_END()
};

size_t load_addr = 0;
size_t size = sizeof(program) / sizeof(ulp_insn_t);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ADC Sample Collection");
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);
    ESP_ERROR_CHECK(ulp_process_macros_and_load(load_addr, program, &size));
    Serial.printf("ULP program occupied %u 32-bit words of RTC_SLOW_MEM.\n", size);

    while (total_samples < MAX_SAMPLES) {
        ESP_ERROR_CHECK(ulp_run(load_addr));

        unsigned long sampleCheckCount = 0;
        while ((RTC_SLOW_MEM[100] & 0xFFFF) < SAMPLE_SIZE) {
            sampleCheckCount++;
            Serial.printf("Current sample count: %d, Check count: %lu\n", (RTC_SLOW_MEM[100] & 0xFFFF), sampleCheckCount);
            delay(10);
        }

        Serial.println("ADC Readings from RTC_SLOW_MEM:");
        for (int i = 0; i < SAMPLE_SIZE; i++) {
            if (total_samples < MAX_SAMPLES) {
                uint16_t raw_value = RTC_SLOW_MEM[100 + i] & 0xFFF;
                float processed_value = log((float)raw_value / REF) * 500 + 30;
                cpu_samples[total_samples++] = processed_value;
                Serial.printf("Measurement %d: %4i\n", total_samples, cpu_samples[total_samples - 1]);

                if (processed_value > GAIN_THRESHOLD) {
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(3000);
                    digitalWrite(BUZZER_PIN, LOW);
                }
            }
        }

        Serial.printf("\nCollected %d samples.\n", total_samples);

        if (total_samples >= MAX_SAMPLES) {
            String jsonData = "[";
            for (size_t i = 0; i < total_samples; i++) {
                jsonData += "{\"sample\":" + String(cpu_samples[i]) + "}";
                if (i < total_samples - 1) {
                    jsonData += ",";
                }
            }
            jsonData += "]";

            Serial.println("Sending the following samples:");
            Serial.println(jsonData);

            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            Serial.print("Connecting to WiFi...");
            while (WiFi.status() != WL_CONNECTED) {
                delay(1000);
                Serial.println("Connecting...");
            }
            Serial.println("Connected to WiFi!");

            if (WiFi.status() == WL_CONNECTED) {
                HTTPClient http;
                http.begin("http://yourserver.com/api/endpoint");
                http.addHeader("Content-Type", "application/json");
                int httpResponseCode = http.POST(jsonData);

                if (httpResponseCode > 0) {
                    Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
                } else {
                    Serial.printf("Error on sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
                }

                http.end();
            }

            memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);
            esp_restart();
        }
    }
}

void loop() {
}
