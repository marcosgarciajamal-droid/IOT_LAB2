#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <Arduino.h>
#include <Client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

class MqttPublisher
{
public:
    explicit MqttPublisher(Client &clientRef);

    bool begin(const char *brokerHost,
               uint16_t brokerPort,
               const char *clientId,
               const char *username = nullptr,
               const char *password = nullptr);

    void loop();

    void setAutoReconnect(bool enabled);

    bool isConnected();

    void addField(const char *key, float value, uint8_t decimals = 2);

    void addField(const char *key, long value);

    void addField(const char *key, bool value);

    void addField(const char *key, const char *value);

    void addTimestamp(unsigned long timestamp);

    void clearFields();

    bool publish(const char *topic, bool retained = false, bool retainFields = false);

//    bool publishReading(const char *topic,
  //                      float temperature,
    //                    float humidity,
      //                  bool retained = false);

    PubSubClient &mqttClient();

private:
    PubSubClient _mqtt;
    Client &_netClient;

    const char *_clientId = nullptr;
    const char *_username = nullptr;
    const char *_password = nullptr;

    bool _autoReconnect = true;

    StaticJsonDocument<MQTT_SENSOR_PUBLISHER_JSON_CAPACITY> _doc;

    bool reconnect();
};

#endif // MQTT_PUBLISHER_H