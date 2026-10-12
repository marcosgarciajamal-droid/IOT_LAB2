/*
 * Test: transmission power vs energy consumption and signal strength
 * (assignment section 5.4: "Changing transmission power to evaluate energy
 *  consumption and signal strength")
 *
 * Runs on the TRANSMITTER board:
 *     pio test -e tx -f test_tx_power
 *
 * For each power level it sends PACKETS_PER_LEVEL packets and measures the
 * time on air. The RECEIVER board (env rx, unchanged) prints the RSSI of each
 * packet on its serial monitor, and the JSON carries the power used:
 *     Received '{"test":"tx_power","power":14,"seq":3}' with RSSI -52
 *
 * Consumption: during each level, read the current with a USB power meter
 * (or a multimeter in series) and compute  E = V · I · t_air  per packet.
 */
#include <Arduino.h>
#include <LoRa.h>
#include <unity.h>
#include <config.h>
#include "LoRaConfig.h"
#include "LoRa_IOT.h"

// LoRa_IOT.cpp uses this flag in its interrupt (in the real firmware it is defined in rx/main.cpp)
volatile bool rxFlag = false;

// Power levels to test in dBm (Heltec V2 PA_BOOST: 2–17 and 20)
const int TX_POWERS[]       = {2, 5, 8, 11, 14, 17, 20};
const int NUM_POWERS        = sizeof(TX_POWERS) / sizeof(TX_POWERS[0]);

const int PACKETS_PER_LEVEL = 10;      // packets per power level
const int PACKET_GAP_MS     = 500;     // pause between packets
const int LEVEL_GAP_MS      = 3000;    // pause between power levels (to read the meter)

LoRaConfig loraConfig;


void test_lora_setup()
    {
        TEST_ASSERT_TRUE_MESSAGE(LoRaIOT_setup(loraConfig), "Could not initialise LoRa");
    }


void test_tx_power_sweep()
    {
        Serial.println();
        Serial.println("power_dBm,sent,failed,avg_time_on_air_ms");

        int total_failed = 0;

        for (int i = 0; i < NUM_POWERS; i++)
            {
                int power = TX_POWERS[i];
                LoRa.setTxPower(power);

                int sent = 0;
                int failed = 0;
                unsigned long total_time = 0;

                for (int seq = 0; seq < PACKETS_PER_LEVEL; seq++)
                    {
                        String message = "{\"test\":\"tx_power\",\"power\":" + String(power) +
                                         ",\"seq\":" + String(seq) + "}";

                        unsigned long t0 = millis();
                        bool ok = LoRaIOT_send(message);
                        total_time += millis() - t0;    // endPacket() waits for TxDone

                        if (ok) { sent++; } else { failed++; }
                        delay(PACKET_GAP_MS);
                    }

                float avg_toa = (float)total_time / PACKETS_PER_LEVEL;

                // One CSV line per power level (easy to paste into a spreadsheet)
                Serial.printf("%d,%d,%d,%.1f\n", power, sent, failed, avg_toa);

                total_failed += failed;
                delay(LEVEL_GAP_MS);
            }

        // Restore the original power from config.h
        LoRa.setTxPower(loraConfig.power);

        TEST_ASSERT_EQUAL_MESSAGE(0, total_failed, "Some packets could not be transmitted");
    }


void setup()
    {
        delay(2000);            // give the test runner time to open the serial port
        Serial.begin(115200);

        UNITY_BEGIN();
        RUN_TEST(test_lora_setup);
        RUN_TEST(test_tx_power_sweep);
        UNITY_END();
    }


void loop()
    {
    }
