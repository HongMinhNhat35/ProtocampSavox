#include "driver/adc.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>
#include <string.h>

#define SAMPLE_SIZE 1000         // Number of samples to collect per batch
#define MAX_SAMPLES 3000         // Maximum number of samples to collect before sending data
#define WIFI_SSID ""        // Wi-Fi SSID (replace with your own)
#define WIFI_PASSWORD ""  // Wi-Fi password (replace with your own)

#define BUZZER_PIN 17            // GPIO pin for the buzzer
#define GAIN_THRESHOLD 1000.0    // Threshold for activating the buzzer
#define DEEP_SLEEP_TIME 3000     // Deep sleep time in microseconds (3 seconds)

uint16_t cpu_samples[MAX_SAMPLES];      // Array to store processed ADC samples
unsigned long timestamps[MAX_SAMPLES];  // Array to store timestamps for each sample
size_t total_samples = 0;               // Total number of samples collected
uint16_t buzzer_flag = 0;

// Define a base time for August 28, 2024, at 13:00 (1 PM) local Finland time (UTC+3)
const unsigned long BASE_TIME = 1693213200000; // Milliseconds since UNIX epoch

void setup() {
    Serial.begin(115200);  // Initialize serial communication
    Serial.println("Starting ADC Sample Collection");

    // Configure the ADC for GPIO 9 (ADC1_CHANNEL_8)
    adc1_config_width(ADC_WIDTH_BIT_12);  // Set ADC width to 12 bits
    adc1_config_channel_atten(ADC1_CHANNEL_8, ADC_ATTEN_DB_11);

    // Set up the buzzer pin as an output
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off initially

    // Main loop to collect and process samples indefinitely
    while (true) {
        // Read and process samples
        Serial.println("Reading ADC samples...");
        for (int i = 0; i < SAMPLE_SIZE; i++) {
            int raw_value = adc1_get_raw(ADC1_CHANNEL_8); // Read raw ADC value

            // Store processed value
            if (total_samples < MAX_SAMPLES) {
                cpu_samples[total_samples] = raw_value;  // Store raw value
                
                // Only track timestamp every sample
                timestamps[total_samples] = BASE_TIME + (total_samples * 10); // Adjusting timestamp based on index
                total_samples++;

                Serial.printf("Measurement %d: %4i at %lu ms\n", total_samples, cpu_samples[total_samples - 1], timestamps[total_samples - 1]);

                // Check if the processed value exceeds the gain threshold
                if (raw_value > GAIN_THRESHOLD && raw_value < 64165.0) {
                    buzzer_flag += 1;
                }
                if (buzzer_flag == 1) {
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(1500);
                    digitalWrite(BUZZER_PIN, LOW);
                }
            }
            delay(10);  // Small delay to avoid flooding the ADC reads
        }
        Serial.printf("\nCollected %d samples.\n", total_samples);

        // Check if the maximum number of samples has been collected
        if (total_samples >= MAX_SAMPLES) {
            // Prepare JSON data with averages of every 50 samples
            String jsonData = "[";
            for (size_t i = 0; i < total_samples; i += 50) {
                if (i + 50 <= total_samples) {
                    // Calculate the average of the next 50 samples
                    float sum = 0;
                    for (size_t j = 0; j < 50; j++) {
                        sum += cpu_samples[i + j];
                    }
                    float average = sum / 50.0;

                    // Apply constraints to the average
                    if (average < 30) {
                        average = 30; // Set minimum to 30
                    } else if (average > 90) {
                        average = 90; // Set maximum to 90
                    }

                    // Create a JSON object for the average, using the timestamp from the first sample of this batch
                    jsonData += "{\"timestamp\":" + String(timestamps[i]) + ",\"average\":" + String(average) + "}";
                    if (i + 50 < total_samples) {
                        jsonData += ",";
                    }
                }
            }
            jsonData += "]";

            // Print the JSON data to be sent
            Serial.println("Sending the following samples:");
            Serial.println(jsonData);

            // Connect to WiFi
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            Serial.print("Connecting to WiFi...");
            int connectionAttempts = 0;
            while (WiFi.status() != WL_CONNECTED && connectionAttempts < 10) {  // Try to connect with a limit
                delay(1000);
                Serial.println("Connecting...");
                connectionAttempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi!");

                // Send HTTP POST request with JSON data to the Node.js server
                HTTPClient http;
                http.begin("http://192.168.50.230:3000/data");  // Replace with your server's IP address and port
                http.addHeader("Content-Type", "application/json");
                int httpResponseCode = http.POST(jsonData);

                // Print the HTTP response code
                if (httpResponseCode > 0) {
                    Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
                    String responseMessage = http.getString();  // Get the response body as a string
                    Serial.println("Response from server:");
                    Serial.println(responseMessage);  // Print the response message
                } else {
                    Serial.printf("Error on sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
                }

                http.end();  // Close the HTTP connection
            } else {
                Serial.println("Failed to connect to WiFi.");
            }
            WiFi.disconnect();
            Serial.println("Disconnected from WiFi");

            // Reset counters and arrays for the next batch of samples
            total_samples = 0;
            memset(cpu_samples, 0, sizeof(cpu_samples));
            memset(timestamps, 0, sizeof(timestamps));

            // Enter deep sleep or any additional logic
        }
    }
}

void loop() {
    // No repeated actions in loop since all logic is in setup
}
