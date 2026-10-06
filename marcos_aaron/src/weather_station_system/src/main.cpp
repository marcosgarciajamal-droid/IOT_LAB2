#include <Arduino.h>
#include <WiFiClient.h>

#include "config.h"
#include "wifi_manager.hh"
#include "dht.hh"
#include "MqttPublisher.hh"

DHTSensor sensor(DHT_PIN, DHT_TYPE);
WiFiManager wifiManager(WIFI_SSID, WIFI_PASSWORD, WIFI_TIMEOUT_MS, WIFI_RETRY_MS);

WiFiClient espClient;
MqttPublisher mqttPublisher(espClient);

unsigned long lastPublish = 0;
bool mqttStarted = false;


bool publishReading(MqttPublisher &publisher,const char *topic,float temperature,float humidity,bool retained = false)
{
    publisher.addField("temp", temperature);
    publisher.addField("hum", humidity);
    return publisher.publish(topic, retained);
}


void setup()
{
  Serial.begin(115200);

  sensor.begin();
  wifiManager.begin();
}

void loop()
{
  wifiManager.update();

  if (!wifiManager.isConnected())
    return;

  if (!mqttStarted)
  {
    mqttStarted = mqttPublisher.begin(MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_CLIENT_ID);
    if (mqttStarted){
      Serial.println("MQTT Connected to the broker");
    }
    else{
      Serial.println("It was not able to connect to the broker MQTT. Next loop");
    }
  }

  mqttPublisher.loop();

  unsigned long now = millis();
  if (now - lastPublish >= MQTT_PUBLISH_INTERVAL_MS){
    lastPublish = now;

    if (sensor.read()){
      float temp = sensor.getTemperature();
      float hum = sensor.getHumidity();

      Serial.print("Temp: ");  //comprobar si esta funcionando la lectura
      Serial.print(temp);
      Serial.print(" C  Hum: ");
      Serial.print(hum);
      Serial.println(" %");

      bool ok = publishReading(mqttPublisher, MQTT_TOPIC, temp, hum);
      Serial.println(ok ? "Published OK" : "Error publishing");
    }
    else
    {
      Serial.println("Error reading DHT11");
    }
  }
}
