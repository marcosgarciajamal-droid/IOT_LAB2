/*
 * ============================================================
 * Connectivity in IoT - Activity 1
 * ESP32 + DHT11 + WiFi + MQTT
 * ============================================================
 *
 * Function:
 *
 *     DHT11
 *       |
 *       v
 *     ESP32
 *       |
 *       | WiFi
 *       v
 *     MQTT Broker (Mosquitto)
 *
 * The ESP32 reads temperature and humidity from the DHT11
 * and publishes both values in JSON format using MQTT.
 *
 * ============================================================
 */


// ============================================================
// LIBRARIES
// ============================================================

#include <Arduino.h>
#include <WiFi.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <PubSubClient.h>


// ============================================================
// DHT11 CONFIGURATION
// ============================================================

// The example provided for the laboratory uses GPIO 2.
#define DHTPIN 2

#define DHTTYPE DHT11

DHT_Unified dht(DHTPIN, DHTTYPE);


// ============================================================
// WIFI CONFIGURATION
// ============================================================

// TODO:
// Replace these values with the WiFi network credentials.

const char* WIFI_SSID = "lab1";

const char* WIFI_PASSWORD = "jaumemika";


// ============================================================
// MQTT CONFIGURATION
// ============================================================

// TODO:
// These values will be filled in when the Mosquitto server
// is configured.
//
// Default MQTT port:
// 1883

const char* MQTT_SERVER = "10.81.100.108";

const int MQTT_PORT = 1883;


// If the MQTT broker does not require authentication,
// leave these as empty strings.

const char* MQTT_USER = "";

const char* MQTT_PASSWORD = "";


// ============================================================
// MQTT TOPIC
// ============================================================

// The temperature and humidity will be sent together
// in a single JSON message.
//
// TODO:
// Change this topic if the laboratory specifies another one.

const char* MQTT_TOPIC = "weatherstation/data";


// ============================================================
// MQTT CLIENT
// ============================================================

WiFiClient espClient;

PubSubClient mqttClient(espClient);


// ============================================================
// SENSOR READING INTERVAL
// ============================================================

// Time between sensor readings.
//
// The DHT11 should not be read too frequently.
// 2000 ms = 2 seconds.

const unsigned long SENSOR_INTERVAL = 2000;

unsigned long lastSensorRead = 0;


// ============================================================
// WIFI CONNECTION
// ============================================================

void connectWiFi()
{
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);

        Serial.print(".");
    }

    Serial.println();

    Serial.println("WiFi connected.");

    Serial.print("ESP32 IP address: ");

    Serial.println(WiFi.localIP());
}


// ============================================================
// MQTT CONNECTION
// ============================================================

void connectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Connecting to MQTT broker... ");

        // Create a unique MQTT client ID using the ESP32 MAC.
        String clientId = "ESP32-";

        clientId += String((uint32_t)ESP.getEfuseMac(), HEX);


        bool connected;


        // ----------------------------------------------------
        // MQTT without authentication
        // ----------------------------------------------------

        if (strlen(MQTT_USER) == 0)
        {
            connected = mqttClient.connect(clientId.c_str());
        }


        // ----------------------------------------------------
        // MQTT with username/password
        // ----------------------------------------------------

        else
        {
            connected = mqttClient.connect(
                clientId.c_str(),
                MQTT_USER,
                MQTT_PASSWORD
            );
        }


        // ----------------------------------------------------
        // Connection result
        // ----------------------------------------------------

        if (connected)
        {
            Serial.println("connected.");
        }
        else
        {
            Serial.print("failed, MQTT state = ");

            Serial.println(mqttClient.state());

            // Wait before trying again.
            delay(5000);
        }
    }
}


// ============================================================
// READ SENSOR AND PUBLISH MQTT MESSAGE
// ============================================================

void readAndPublishSensor()
{
    sensors_event_t temperatureEvent;

    sensors_event_t humidityEvent;


    // --------------------------------------------------------
    // Read temperature
    // --------------------------------------------------------

    dht.temperature().getEvent(&temperatureEvent);


    if (isnan(temperatureEvent.temperature))
    {
        Serial.println("Error reading temperature.");

        return;
    }


    // --------------------------------------------------------
    // Read humidity
    // --------------------------------------------------------

    dht.humidity().getEvent(&humidityEvent);


    if (isnan(humidityEvent.relative_humidity))
    {
        Serial.println("Error reading humidity.");

        return;
    }


    // --------------------------------------------------------
    // Display values on Serial Monitor
    // --------------------------------------------------------

    Serial.print("Temperature: ");

    Serial.print(temperatureEvent.temperature);

    Serial.println(" °C");


    Serial.print("Humidity: ");

    Serial.print(humidityEvent.relative_humidity);

    Serial.println(" %");


    // --------------------------------------------------------
    // Create JSON message
    // --------------------------------------------------------
    //
    // Example:
    //
    // {
    //   "temperature": 23.50,
    //   "humidity": 54.00
    // }
    //
    // --------------------------------------------------------

    char jsonMessage[100];


    snprintf(
        jsonMessage,
        sizeof(jsonMessage),
        "{\"temperature\":%.2f,\"humidity\":%.2f}",
        temperatureEvent.temperature,
        humidityEvent.relative_humidity
    );


    // --------------------------------------------------------
    // Publish JSON message using MQTT
    // --------------------------------------------------------

    Serial.print("Publishing MQTT message: ");

    Serial.println(jsonMessage);


    bool published = mqttClient.publish(
        MQTT_TOPIC,
        jsonMessage
    );


    // --------------------------------------------------------
    // Check publication
    // --------------------------------------------------------

    if (published)
    {
        Serial.println("MQTT message published successfully.");
    }
    else
    {
        Serial.println("ERROR: MQTT message could not be published.");
    }


    Serial.println("--------------------------------");
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Serial communication
    // --------------------------------------------------------

    Serial.begin(115200);

    delay(1000);


    Serial.println();

    Serial.println("========================================");

    Serial.println("Connectivity in IoT - Activity 1");

    Serial.println("ESP32 + DHT11 + MQTT");

    Serial.println("========================================");


    // --------------------------------------------------------
    // Initialize DHT11
    // --------------------------------------------------------

    dht.begin();

    Serial.println("DHT11 initialized.");


    // --------------------------------------------------------
    // Configure MQTT server
    // --------------------------------------------------------

    mqttClient.setServer(
        MQTT_SERVER,
        MQTT_PORT
    );


    // --------------------------------------------------------
    // Connect to WiFi
    // --------------------------------------------------------

    connectWiFi();


    // --------------------------------------------------------
    // Connect to MQTT broker
    // --------------------------------------------------------

    connectMQTT();


    Serial.println();

    Serial.println("System ready.");

    Serial.println();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // Check WiFi connection
    // --------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi connection lost.");

        connectWiFi();
    }


    // --------------------------------------------------------
    // Check MQTT connection
    // --------------------------------------------------------

    if (!mqttClient.connected())
    {
        Serial.println("MQTT connection lost.");

        connectMQTT();
    }


    // Keep MQTT connection alive.
    mqttClient.loop();


    // --------------------------------------------------------
    // Read sensor periodically
    // --------------------------------------------------------

    unsigned long currentMillis = millis();


    if (currentMillis - lastSensorRead >= SENSOR_INTERVAL)
    {
        lastSensorRead = currentMillis;

        readAndPublishSensor();
    }
}