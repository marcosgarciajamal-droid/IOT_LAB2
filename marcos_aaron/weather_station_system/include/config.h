
#ifndef CONFIG_H
#define CONFIG_H


#define DHT_PIN   13     
#define DHT_TYPE  DHT11  

#define WIFI_SSID     "R2-D2"
#define WIFI_PASSWORD "marijose132"
#define WIFI_TIMEOUT_MS 10000  
#define WIFI_RETRY_MS     5000    // espera entre reintentos tras un fallo



#define MQTT_BROKER_HOST  "10.200.211.159"  
#define MQTT_BROKER_PORT  1883
#define MQTT_CLIENT_ID    "heltec_esp32_01"
#define MQTT_TOPIC         "sensor/1"
#define MQTT_PUBLISH_INTERVAL_MS 5000 // publicamos un mensaje mqtt cada 5 segundos


#define MQTT_SENSOR_PUBLISHER_MAX_FIELDS 10
#define MQTT_SENSOR_PUBLISHER_JSON_CAPACITY 256

#endif 
