#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <Arduino.h>
#include <DHT.h>

class DHTSensor {
  public:
    DHTSensor(uint8_t pin, uint8_t type);

    void begin();

    // Devuelve true si la lectura fue válida, false si hubo error
    bool read();

    float getTemperature() const;
    float getHumidity() const;

  private:
    DHT _dht;
    float _temperature;
    float _humidity;
};

#endif