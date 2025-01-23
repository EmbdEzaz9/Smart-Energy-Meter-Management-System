#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Wi-Fi Configuration
#define WLAN_SSID       "OnePlus 7"      // Wi-Fi Network Name
#define WLAN_PASS       "Tanhaie84"     // Wi-Fi Password

// Adafruit IO MQTT Configuration
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883             // Use 8883 for SSL
#define AIO_USERNAME    "EZAZ"           // Adafruit IO Username
#define AIO_KEY         "provide your secret key"  // Adafruit IO Key

// Sensor Configuration
#define SENSOR_PIN      A0               // Analog pin connected to the sensor
#define MV_PER_AMP      66               // Sensitivity: 66mV for 20A/30A sensor modules

// Create an ESP8266 WiFi client
WiFiClient client;

// Create an Adafruit MQTT client
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Create a feed for publishing current data
Adafruit_MQTT_Publish currentFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/current");

// Global variables for current measurement
double Voltage = 0;
double Vrms = 0;
double Irms = 0;

// Function Prototypes
void MQTT_connect();
double getVoltagePeakToPeak();

void setup() {
  Serial.begin(9600);
  
  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Connected to Wi-Fi!");
}

void loop() {
  // Ensure MQTT connection is active
  MQTT_connect();

  // Measure Voltage and Current
  Voltage = getVoltagePeakToPeak();
  Vrms = (Voltage / 2.0) * 0.707;  // Convert to RMS
  Irms = ((Vrms * 1000) / MV_PER_AMP);  // Calculate Current

  // Print current to Serial Monitor
  Serial.print("Current: ");
  Serial.print(Irms);
  Serial.println(" Amps");

  // Publish current to Adafruit IO
  if (!currentFeed.publish(Irms)) {
    Serial.println("Publish Failed!");
  } else {
    Serial.println("Publish Successful!");
  }

  // Wait before the next reading
  delay(2000);
}

/**
 * @brief Measures the peak-to-peak voltage over a 1-second interval.
 * @return Peak-to-peak voltage in volts.
 */
double getVoltagePeakToPeak() {
  int readValue;      // Current sensor reading
  int maxValue = 0;   // Maximum sensor value
  int minValue = 1024; // Minimum sensor value

  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) {  // Sample for 1 second
    readValue = analogRead(SENSOR_PIN);
    if (readValue > maxValue) maxValue = readValue;  // Update max value
    if (readValue < minValue) minValue = readValue;  // Update min value
  }

  // Convert peak-to-peak value to voltage
  double result = ((maxValue - minValue) * 5.0) / 1024.0;
  return result;
}

/**
 * @brief Ensures the device is connected to the MQTT server.
 */
void MQTT_connect() {
  if (mqtt.connected()) {
    Serial.println("MQTT is already connected.");
    return;
  }

  Serial.print("Connecting to MQTT...");
  uint8_t retries = 3;
  while (mqtt.connect() != 0) {
    Serial.println(mqtt.connectErrorString(mqtt.connect()));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
    if (--retries == 0) {
      Serial.println("MQTT connection failed! Restarting...");
      while (1);  // Wait for watchdog timer to reset
    }
  }
  Serial.println("MQTT Connected!");
}
