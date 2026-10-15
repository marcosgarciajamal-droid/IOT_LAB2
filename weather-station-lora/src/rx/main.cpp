#include <Arduino.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <LoRaConfig.h>
#include <ArduinoJson.h>
#include "LoRa_IOT.h"
#include "config.h"
#include "wifi_manager.hh"
#include "MqttPublisher.hh"

WiFiManager wifiManager(
    WIFI_SSID,
    WIFI_PASSWORD,
    WIFI_TIMEOUT_MS,
    WIFI_RETRY_MS
);

WiFiClient espClient;
MqttPublisher mqttPublisher(espClient);

bool mqttStarted = false;

bool publishReading(MqttPublisher &publisher, const char *topic, const String &message, bool retained = false)
{
    StaticJsonDocument<200> doc; // Parse Json
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.print("ERROR: JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }

    float temperature = doc["temp"];
    float humidity = doc["hum"];

    publisher.addField("temp", temperature);
    publisher.addField("hum", humidity);

    bool result = publisher.publish(topic, retained);

    if (result)
    {
        Serial.println("MQTT publish OK");
    }
    else
    {
        Serial.println("MQTT publish FAILED");
    }

    return result;
}

void setup()
{
    Serial.begin(115200);

    wifiManager.begin();

    if (!LoRaIOT_setup(LoRaConfig()))
    {
        Serial.println("Starting LoRa failed Version 2!");
        while (1);
    }

    LoRa.receive();
}

void loop()
{
    wifiManager.update();

    if (!wifiManager.isConnected())
        return;

    if (!mqttStarted)
    {
        mqttStarted = mqttPublisher.begin(
            MQTT_BROKER_HOST,
            MQTT_BROKER_PORT,
            MQTT_CLIENT_ID
        );

        if (mqttStarted)
        {
            Serial.println("MQTT Connected to the broker");
        }
        else
        {
            Serial.println(
                "It was not able to connect to the broker MQTT. Next loop"
            );
        }
    }

    mqttPublisher.loop();

    if (rxFlag)
    {
        rxFlag = false;
        String message = "";
        if (LoRaIOT_receive(message))
        {
            Serial.println("LoRa message received correctly");
            if (mqttStarted)
            {
                if (publishReading(mqttPublisher, MQTT_TOPIC, message))
                {
                    Serial.println("Published OK");
                }
                else
                {
                    Serial.println("Error publishing");
                }
            }
        }
        else
        {
            Serial.println("LoRa interrupt received, but no packet available");
        }
    }
}