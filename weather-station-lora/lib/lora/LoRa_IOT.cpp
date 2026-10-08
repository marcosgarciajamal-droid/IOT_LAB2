#include "LoRa_IOT.h"

#include <SPI.h>
#include <LoRa.h>


bool LoRaIOT_setup(const LoRaConfig &config)
    {
        // Pines SPI y del módulo LoRa
        SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, config.nss_pin);
        LoRa.setPins(config.nss_pin, config.rst_pin, config.dio0_pin);

        if (!LoRa.begin(config.frequency))
            {
                Serial.println("Starting LoRa failed!");
                return false;
            }

        // Parámetros de radio
        LoRa.setSpreadingFactor(config.sf);
        LoRa.setSignalBandwidth(config.bandwidth);
        LoRa.setCodingRate4(config.cr + 4);     // 1 → 4/5, 2 → 4/6...
        LoRa.setPreambleLength(config.preamble_len);
        LoRa.setSyncWord(config.sync_word);
        LoRa.setTxPower(config.power);
        LoRa.enableCrc();

        Serial.println("LoRa init OK");
        return true;
    }


bool LoRaIOT_send(const String &message)
    {
        LoRa.beginPacket();
        LoRa.print(message);
        return LoRa.endPacket() == 1;
    }


bool LoRaIOT_receive(String &message)
    {
        int packetSize = LoRa.parsePacket();
        if (packetSize == 0) { return false; }

        message = "";
        while (LoRa.available())
            {
                message += (char)LoRa.read();
            }

        Serial.print("Received '");
        Serial.print(message);
        Serial.print("' with RSSI ");
        Serial.println(LoRa.packetRssi());

        return true;
    }
