#include <config.h>
#include <LoRa.h>
#include <LoRaConfig.h>

void IRAM_ATTR onLoRaRxIRQ()
    {
        BaseType_t woken = pdFALSE;
        //xSemaphoreGiveFromISR(_rx_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }


bool beginLoRa(const LoRaConfig &config)
    {
        // Set LoRa pins
        LoRa.setPins(config.nss_pin, config.rst_pin, config.dio0_pin);

        //_rx_sem = xSemaphoreCreateBinary();
        attachInterrupt(digitalPinToInterrupt(config.dio0_pin), onLoRaRxIRQ, RISING);

        // LoRa Module Reset
        pinMode(config.rst_pin, OUTPUT);
        digitalWrite(config.rst_pin, LOW);
        delay(10);
        digitalWrite(config.rst_pin, HIGH);
        delay(10);

        // LoRa Initialization
        if (!LoRa.begin(config.frequency)) { return false; }

        // LoRa Configuration
        LoRa.setSpreadingFactor(config.sf);
        LoRa.setSignalBandwidth(config.bandwidth);
        LoRa.setCodingRate4(config.cr);
        LoRa.setPreambleLength(config.preamble_len);
        LoRa.setTxPower(config.power);

        LoRa.receive();

        return true;
    }

