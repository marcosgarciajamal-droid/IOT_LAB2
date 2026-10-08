#include <Arduino.h>
#include <DHT.h>
#include <config.h>
#include "LoRaConfig.h"
#include "LoRa_IOT.h"


DHT dht(DHT_PIN, DHT_TYPE);
LoRaConfig loraConfig;                  // valores por defecto de config.h

int counter = 0;


void setup()
    {
        Serial.begin(115200);
        while (!Serial);

        Serial.println("LoRa Sender");

        dht.begin();

        if (!LoRaIOT_setup(loraConfig))
            {
                while (1);
            }
    }


void loop()
    {
        float temperature = dht.readTemperature();
        float humidity    = dht.readHumidity();

        if (isnan(temperature) || isnan(humidity))
            {
                Serial.println("Error leyendo el DHT11");
                delay(2000);
                return;
            }

        // Mensaje en JSON: {"id":0,"temperature":23.4,"humidity":55.0}
        String message = "{\"id\":" + String(counter) +
                         ",\"temperature\":" + String(temperature, 1) +
                         ",\"humidity\":" + String(humidity, 1) + "}";

        Serial.print("Sending packet: ");
        Serial.println(message);

        if (!LoRaIOT_send(message))
            {
                Serial.println("Error enviando el paquete");
            }

        counter++;
        delay(LORA_SEND_INTERVAL_MS);
    }
