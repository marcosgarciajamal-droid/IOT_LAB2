#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

enum class WiFiState {
  DISCONNECTED,
  CONNECTING,
  CONNECTED
};

class WiFiManager {
  public:
    WiFiManager(const char* ssid, const char* password, unsigned long timeoutMs, unsigned long retryMs);

    void begin();          // arranca el proceso (pasa a CONNECTING)
    void update();          // llamar SIEMPRE en loop(), sin delays
    bool isConnected() const;
    WiFiState getState() const;
    IPAddress getIP() const;

  private:
    const char* _ssid;
    const char* _password;
    unsigned long _timeoutMs;
    unsigned long _retryMs;

    WiFiState _state;
    unsigned long _stateStartTime;

    void goTo(WiFiState newState);
};

#endif