#include "driver/adc.h"
#include "esp32/ulp.h"
#include "soc/rtc_cntl_reg.h"
#include <string.h> // Include for memset function
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h> // Include for log() function

#define SAMPLE_SIZE 1000        // Number of samples to collect in each ULP run
#define MAX_SAMPLES 3000        // Maximum number of samples to collect before sending data
#define WIFI_SSID "aalto open" // Wi-Fi SSID (replace with your own)
#define WIFI_PASSWORD ""        // Wi-Fi password (replace with your own)
#define REF 30             // Reference value for logarithm calculation
#define BUZZER_PIN 17           // GPIO pin for the buzzer
#define GAIN_THRESHOLD 90.0     // Threshold for activating the buzzer
#define DEEP_SLEEP_TIME 3000 // Deep sleep time in microseconds

// Use RTC_DATA_ATTR to retain the counter across deep sleeps
RTC_DATA_ATTR uint32_t counter = 0; // Counter variable stored in RTC memory

uint16_t cpu_samples[MAX_SAMPLES];   // Array to store processed ADC samples
unsigned long timestamps[MAX_SAMPLES]; // Array to store timestamps for each sample
size_t total_samples = 0;            // Total number of samples collected

// ULP (Ultra-Low Power) program to read analog values from the microphone
const ulp_insn_t program[] = {
    I_MOVI  (R1, 0),            // Initialize R1 to point to RTC_SLOW_MEM
    I_MOVI  (R2, 0),            // Initialize sample counter (R2) to 0

    M_LABEL (1),                // Start of outer loop
        I_MOVI  (R0, 20),       // Initialize delay counter (R0) to 20
        M_LABEL (2),            // Start of inner loop
            I_DELAY (4000),     // Delay for a specified time (4000 cycles)
            I_SUBI  (R0, R0, 1), // Decrement the delay counter
        M_BGE (2, 1),           // Repeat inner loop while R0 > 0

        I_ADC   (R3, 0, 6),     // Read ADC value from GPIO 34 (ADC1_CHANNEL_6)
        I_ADDI  (R2, R2, 1),    // Increment sample counter (R2)
        I_ST    (R3, R2, 100),  // Store ADC reading in RTC_SLOW_MEM at R2 + 100
        I_ST    (R2, R1, 100),  // Store sample count in RTC_SLOW_MEM[100]

    I_MOVR  (R0, R2),           // Move sample count to R0 for comparison
    M_BL    (1, SAMPLE_SIZE),   // Loop back if sample count (R0) < SAMPLE_SIZE

    I_END()                     // End of ULP program
};

size_t load_addr = 0; // Load address for ULP program
size_t size = sizeof(program) / sizeof(ulp_insn_t); // Calculate the size of ULP program

void setup() {
    Serial.begin(115200); // Initialize serial communication
    Serial.println("Starting ADC Sample Collection");

    // Configure the ADC for the microphone on GPIO 34
    adc1_config_width(ADC_WIDTH_BIT_12); // Set ADC width to 12 bits
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // Set attenuation

    // Set up the buzzer pin as an output
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially

    // Clear RTC_SLOW_MEM before loading ULP program
    memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);

    // Load ULP program into memory
    ESP_ERROR_CHECK(ulp_process_macros_and_load(load_addr, program, &size));
    Serial.printf("ULP program occupied %u 32-bit words of RTC_SLOW_MEM.\n", size);

    // Main loop to collect and process samples indefinitely
    while (true) {
        ESP_ERROR_CHECK(ulp_run(load_addr));

        // Output the measurements and process each sample
        Serial.println("ADC Readings from RTC_SLOW_MEM:");
        for (int i = 0; i < SAMPLE_SIZE; i++) {
            while ((RTC_SLOW_MEM[100] & 0xFFFF) < SAMPLE_SIZE) {
                delay(10); // Small delay between checks
            }
            if (total_samples < MAX_SAMPLES) {
                uint16_t raw_value = RTC_SLOW_MEM[100 + i] & 0xFFF; // Extract raw ADC reading
                // Perform logarithmic transformation on the ADC reading
                float processed_value = log((float)raw_value / REF) * 100 + 30;
                cpu_samples[total_samples] = processed_value; // Store processed value
                timestamps[total_samples] = millis(); // Store current timestamp in milliseconds
                total_samples++;

                Serial.printf("Measurement %d: %4i at %lu ms\n", total_samples, cpu_samples[total_samples - 1], timestamps[total_samples - 1]);
                delay(100);
                // Check if the processed value exceeds the gain threshold
                if (processed_value > GAIN_THRESHOLD && processed_value < 64165.0) {
                    digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
                    delay(3000); // Keep buzzer on for 3 seconds
                    digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
                }
            }
        }

        Serial.printf("\nCollected %d samples.\n", total_samples);
        // Check if the maximum number of samples has been collected
        if (total_samples >= MAX_SAMPLES) {
            // Increment the counter and save it in RTC memory
            counter++;
            RTC_SLOW_MEM[COUNTER_ADDRESS] = counter;

            // Prepare JSON data with timestamps and values
            String jsonData = "[";
            // Add the counter value as the first entry in the JSON array
            jsonData += "{\"counter\":" + String(counter) + "},";
            for (size_t i = 0; i < total_samples; i++) {
                jsonData += "{\"timestamp\":" + String(timestamps[i]) + ",\"value\":" + String(cpu_samples[i]) + "}";
                if (i < total_samples - 1) {
                    jsonData += ",";
                }
            }
            jsonData += "]";

            // Print the JSON data to be sent
            Serial.println("Sending the following samples:");
            Serial.println(jsonData);

            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            Serial.print("Connecting to WiFi...");
            int connectionAttempts = 0;
            while (WiFi.status() != WL_CONNECTED && connectionAttempts < 10) { // Try to connect with a limit
                delay(1000);
                Serial.println("Connecting...");
                connectionAttempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi!");

                // Send HTTP POST request with JSON data
                HTTPClient http;
                http.begin("http://yourserver.com/api/endpoint"); // Replace with your server's URL
                http.addHeader("Content-Type", "application/json");
                int httpResponseCode = http.POST(jsonData);

                // Print the HTTP response code
                if (httpResponseCode > 0) {
                    Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
                } else {
                    Serial.printf("Error on sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
                }

                http.end(); // Close the HTTP connection
            } else {
                Serial.println("Failed to connect to WiFi.");
            }
            WiFi.disconnect();
            Serial.println("Disconnected from WiFi");

            // Reset counters and arrays for the next batch of samples
            total_samples = 0;
            memset(cpu_samples, 0, sizeof(cpu_samples));
            memset(timestamps, 0, sizeof(timestamps));
            memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);

            // Configure deep sleep
            Serial.println("Entering deep sleep...");
            esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME); // Set wakeup timer
            esp_deep_sleep_start(); // Enter deep sleep
        }
    }
}

void loop() {
    // No repeated actions in loop since all logic is in setup
}
