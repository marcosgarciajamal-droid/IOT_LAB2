#include "dht.hh"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type)
  : _dht(pin, type), _temperature(NAN), _humidity(NAN) {
}

void DHTSensor::begin() {
  _dht.begin();
}

bool DHTSensor::read() {
  float h = _dht.readHumidity();
  float t = _dht.readTemperature(); // Celsius

  if (isnan(h) || isnan(t)) {
    return false; // lectura fallida, no se actualizan los valores
  }

  _humidity = h;
  _temperature = t;
  return true;
}

float DHTSensor::getTemperature() const {  //return thte last value read with the previous functions.
  return _temperature;
}

float DHTSensor::getHumidity() const {
  return _humidity;
}