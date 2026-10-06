#include <Arduino.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <LoRaConfig.h>
#include <LoRaDriver.hh>

#include "config.h"
#include "wifi_manager.hh"
#include "MqttPublisher.hh"

WiFiManager wifiManager(WIFI_SSID, WIFI_PASSWORD, WIFI_TIMEOUT_MS, WIFI_RETRY_MS);

WiFiClient espClient;
MqttPublisher mqttPublisher(espClient);

unsigned long lastPublish = 0;
bool mqttStarted = false;


bool publishReading(MqttPublisher &publisher,const char *topic, float temperature, float humidity, bool retained = false)
{
    publisher.addField("temp", temperature);
    publisher.addField("hum", humidity);

    return publisher.publish(topic, retained);
}


void setup()
{
    Serial.begin(115200);

    wifiManager.begin();

    // Inicializar LoRa
    if (!beginLoRa(LoRaConfig())) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    Serial.println("LoRa Receiver");
}


void loop()
{
    wifiManager.update();

    if (!wifiManager.isConnected())
        return;

    // Conectar al broker MQTT
    if (!mqttStarted)
    {
        mqttStarted = mqttPublisher.begin(MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_CLIENT_ID);

        if (mqttStarted)
        {
            Serial.println("MQTT Connected to the broker");
        }
        else
        {
            Serial.println("It was not able to connect to the broker MQTT. Next loop");
        }
    }

    mqttPublisher.loop();


    // Comprobar si ha llegado un paquete LoRa
    int packetSize = LoRa.parsePacket();

    if (packetSize)
    {
        String message = "";

        while (LoRa.available())
        {
            message += (char)LoRa.read();
        }

        Serial.print("Received packet: ");
        Serial.println(message);

        Serial.print("RSSI: ");
        Serial.println(LoRa.packetRssi());

        // De momento solo mostramos el mensaje.
        // Después aquí extraeremos temperatura y humedad.
    }
}