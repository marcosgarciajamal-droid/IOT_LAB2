
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


// =============================
// === RF LORA CONFIGURATION ===
// =============================

#define EU868_FREQUENCY             868000000L      // Hz
#define BANDWIDTH_250_KHZ           250E3           // Hz; 250 kHz used for LONG_FAST profile
#define A3MESH_PREAMBLE_LENGTH      0x08            // symbols
#define A3MESH_SF                   0x0B            // [6–12]
#define A3MESH_CR                   0x01            // 1 = 4/5, 2 = 4/6, 3 = 4/7, 4 = 4/8
#define A3MESH_SYNC_WORD            0x12            // 0x34 is reserved for LoRaWAN
#define A3MESH_POWER                0x14            // dBm [2–17]
#define LORA_MAX_PACKET_SIZE        256


// ==========================================
// === LORA CHIP PINS - SX127x (A3MESH TFG) ==
// ==========================================
// Wiring of the ESP32 DevKit board used by the A3MESH firmware (src/a3mesh).
// The LoRa lab firmware runs on a Heltec V3 and uses the pins declared in the
// HELTEC WIFI LORA 32 V3 section below instead.

#define SCK_GPIO                    0x05
#define MISO_GPIO                   0x0F
#define MOSI_GPIO                   0x19
#define NSS_GPIO                    0x12
#define RST_GPIO                    0x0E
#define DIO0_GPIO                   0x1A






#endif 
