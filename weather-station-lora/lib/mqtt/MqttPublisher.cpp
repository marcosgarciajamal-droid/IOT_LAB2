#include "MqttPublisher.hh"

MqttPublisher::MqttPublisher(Client& clientRef)
    : _mqtt(clientRef), _netClient(clientRef) {
}

bool MqttPublisher::begin(const char* brokerHost, uint16_t brokerPort,const char* clientId, const char* username, const char* password) {
    _clientId = clientId;
    _username = username;
    _password = password;

    _mqtt.setServer(brokerHost, brokerPort);

    return reconnect();
}

bool MqttPublisher::reconnect() {
    if (_mqtt.connected()) {
        return true;
    }

    bool connected;
    if (_username != nullptr) {
        connected = _mqtt.connect(_clientId, _username, _password);
    } else {
        connected = _mqtt.connect(_clientId);
    }

    return connected;
}

void MqttPublisher::setAutoReconnect(bool enabled) {
    _autoReconnect = enabled;
}

bool MqttPublisher::isConnected() {
    return _mqtt.connected();
}

void MqttPublisher::loop() {
    if (_autoReconnect && !_mqtt.connected()) {
        reconnect();
    }
    _mqtt.loop();
}

void MqttPublisher::addField(const char* key, float value, uint8_t decimals) {
    char buf[16];
    dtostrf(value, 0, decimals, buf);
    _doc[key] = atof(buf);
}

void MqttPublisher::addField(const char* key, long value) {
    _doc[key] = value;
}

void MqttPublisher::addField(const char* key, bool value) {
    _doc[key] = value;
}

void MqttPublisher::addField(const char* key, const char* value) {
    _doc[key] = value;
}

void MqttPublisher::addTimestamp(unsigned long timestamp) {
    _doc["timestamp"] = timestamp;
}

void MqttPublisher::clearFields() {
    _doc.clear();
}

bool MqttPublisher::publish(const char* topic, bool retained, bool retainFields) {
    if (!_mqtt.connected()) {
        if (!reconnect()) {
            if (!retainFields) {
                clearFields();
            }
            return false;
        }
    }

    char payload[MQTT_SENSOR_PUBLISHER_JSON_CAPACITY];
    size_t len = serializeJson(_doc, payload, sizeof(payload));

    bool ok = _mqtt.publish(topic, (const uint8_t*)payload, len, retained);

    if (!retainFields) {
        clearFields();
    }

    return ok;
}

//bool MqttPublisher::publishReading(const char* topic, float temperature, float humidity, bool retained) {
  //  addField("temp", temperature);
    //addField("hum", humidity);
    //return publish(topic, retained);
//}

PubSubClient& MqttPublisher::mqttClient() {
    return _mqtt;
}