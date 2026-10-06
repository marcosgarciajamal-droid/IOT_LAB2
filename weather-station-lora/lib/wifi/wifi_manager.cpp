#include "wifi_manager.hh"

WiFiManager::WiFiManager(const char* ssid, const char* password, unsigned long timeoutMs, unsigned long retryMs)
  : _ssid(ssid), _password(password),
    _timeoutMs(timeoutMs), _retryMs(retryMs),
    _state(WiFiState::DISCONNECTED), _stateStartTime(0) {
}

void WiFiManager::goTo(WiFiState newState) {
  _state = newState;
  _stateStartTime = millis();
}

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  goTo(WiFiState::DISCONNECTED); // update() se encaencarga de arrancar CONNECTING
}

void WiFiManager::update() {
  unsigned long elapsed = millis() - _stateStartTime;

  switch (_state) {

    case WiFiState::DISCONNECTED:
      // Si ya pasó el tiempo de espera (o es la primera vez), intenta conectar
      if (_stateStartTime == 0 || elapsed >= _retryMs) {
        Serial.print("Connecting to WiFi: ");
        Serial.println(_ssid);
        WiFi.begin(_ssid, _password);
        goTo(WiFiState::CONNECTING);
      }
      break;

    case WiFiState::CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi Connected. IP: ");
        Serial.println(WiFi.localIP());
        goTo(WiFiState::CONNECTED);
      } else if (elapsed >= _timeoutMs) {
        Serial.println("Timeout connectng to WiFi, Retrying...");
        WiFi.disconnect();
        goTo(WiFiState::DISCONNECTED);
      }
      break;

    case WiFiState::CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Lost connection WiFi");
        goTo(WiFiState::DISCONNECTED);
      }
      break;
  }
}

bool WiFiManager::isConnected() const {
  return _state == WiFiState::CONNECTED;
}

WiFiState WiFiManager::getState() const {
  return _state;
}

IPAddress WiFiManager::getIP() const {
  return WiFi.localIP();
}