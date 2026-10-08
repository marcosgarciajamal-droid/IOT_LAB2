#include <Arduino.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <LoRaConfig.h>
#include <LoRaDriver.hh>
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

unsigned long lastPublish = 0;
bool mqttStarted = false;

volatile bool rxFlag = false;


bool publishReading( MqttPublisher &publisher,const char *topic, const String &message, bool retained = false)
{
    // Mostrar el mensaje LoRa recibido
    Serial.print("Message received: ");
    Serial.println(message);

    // Parsear JSON
    StaticJsonDocument<200> doc;

    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.print("ERROR: JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }

    Serial.println("JSON parsed correctly");

    // Obtener temperatura y humedad
    float temperature = doc["temperature"];
    float humidity = doc["humidity"];

    Serial.print("Temperature: ");
    Serial.println(temperature);

    Serial.print("Humidity: ");
    Serial.println(humidity);


    publisher.addField("temp", temperature);
    publisher.addField("hum", humidity);

    Serial.println(topic);

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
    // Actualizar el estado de la conexión WiFi
    wifiManager.update();

    // Si todavía no hay conexión WiFi, no continuamos
    if (!wifiManager.isConnected())
        return;

    // Si todavía no estamos conectados al broker MQTT,
    // intentamos establecer la conexión
    if (!mqttStarted)
    {
        mqttStarted = mqttPublisher.begin(MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_CLIENT_ID );

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

    // Mantener la conexión MQTT activa
    mqttPublisher.loop();

    if (rxFlag)
    {
        // Limpiar el flag
        rxFlag = false;

        String message = "";

        // Intentar recibir el paquete
        if (LoRaIOT_receive(message))
        {
            Serial.println("LoRa message received correctly");

            // Publicar solamente si hemos recibido un mensaje
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