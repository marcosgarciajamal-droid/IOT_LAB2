#pragma once
#include <stdint.h>
#include <config.h>



struct LoRaConfig
    {
        /*!< SPI chip-select (NSS) GPIO pin connected to the SX127x. */
        uint8_t nss_pin = NSS_GPIO;

        /*!< Hardware reset GPIO pin connected to the SX127x. */
        uint8_t rst_pin = RST_GPIO;

        /*!< DIO0 interrupt GPIO pin connected to the SX127x. */
        uint8_t dio0_pin = DIO0_GPIO;

        /*!< RF centre frequency in Hz (e.g. 868000000 for EU868). */
        long frequency = EU868_FREQUENCY;

        /*!< Signal bandwidth in Hz (e.g. 250E3 for 250 kHz). */
        float bandwidth = BANDWIDTH_250_KHZ;

        /*!< LoRa spreading factor [6–12]. Higher values increase range at the
         *   cost of lower data rate and longer time-on-air. */
        uint8_t sf = A3MESH_SF;

        /*!< Coding rate denominator [1–4] where 1 → 4/5, 2 → 4/6, etc.
         *   Higher values add more FEC overhead and improve resilience. */
        uint8_t cr = A3MESH_CR;

        /*!< LoRa sync word used to distinguish this network from others.
         *   Value 0x34 is reserved for LoRaWAN and must not be used here. */
        uint8_t sync_word = A3MESH_SYNC_WORD;

        /*!< TX output power in dBm [2–17]. */
        int8_t power = A3MESH_POWER;

        /*!< Preamble length in symbols [6–65535].
         *   The effective preamble is 4.25 symbols longer than this value. */
        uint16_t preamble_len = A3MESH_PREAMBLE_LENGTH;
    };

