/**
 * Experiment: power_tx
 * "Changing transmission power to evaluate energy consumption and signal strength."
 *
 * Runs on the TRANSMITTER board. The RECEIVER board runs the normal firmware
 * (env rx) and prints the RSSI of every packet:
 *     Received '{"test":"tx_power","power":14,"seq":3}' with RSSI -52
 *
 * TX sweeps the TX_POWERS table over and over. For every power level it sends
 * PACKETS_PER_LEVEL packets and prints the measured time-on-air of each one
 * plus a summary at the end of the level.
 *
 * Consumption: during each level, read the current with a USB power meter
 * (or a multimeter in series) and compute  E = V * I * t_air  per packet.
 *
 *   pio run -e power_tx -t upload
 *   pio device monitor -e power_tx
 */

#include <Arduino.h>
#include <LoRa.h>
#include <config.h>
#include "LoRaConfig.h"
#include "LoRa_IOT.h"

// =============================
// === EXPERIMENT PARAMETERS ===
// =============================

// Power levels to test in dBm (Heltec V2 PA_BOOST: 2-17 and 20)
const int TX_POWERS[] = {2, 5, 8, 11, 14, 17, 20};
const int NUM_POWERS = sizeof(TX_POWERS) / sizeof(TX_POWERS[0]);

#define PACKETS_PER_LEVEL 10 // packets sent at every power level
#define PACKET_GAP_MS 500    // pause between packets
#define LEVEL_GAP_MS 3000    // pause between power levels (to read the meter)

// LoRa_IOT.cpp uses this flag in its interrupt (in the real firmware it is defined in rx/main.cpp)
volatile bool rxFlag = false;

LoRaConfig loraConfig;

int sweep = 0;

/** JSON payload sent by TX, carries the power used. */
String buildPayload(int power, int seq)
{
    return "{\"test\":\"tx_power\",\"power\":" + String(power) + ",\"seq\":" + String(seq) + "}";
}

/** Sends PACKETS_PER_LEVEL packets at the given power and prints a summary. */
void runLevel(int power)
{
    LoRa.setTxPower(power);

    Serial.println();
    Serial.printf("=== Sweep %d | Power %d dBm | %d packets ===\n", sweep, power, PACKETS_PER_LEVEL);

    int sent = 0;
    float airtimeSum = 0;

    for (int seq = 0; seq < PACKETS_PER_LEVEL; seq++)
    {
        String message = buildPayload(power, seq);

        unsigned long start = micros();
        bool ok = LoRaIOT_send(message); // blocks until TxDone
        float measuredMs = (micros() - start) / 1000.0f;

        Serial.printf("[TX|%ddBm] %s", power, message.c_str());
        if (ok)
        {
            Serial.printf(" | airtime %.1f ms\n", measuredMs);
            sent++;
            airtimeSum += measuredMs;
        }
        else
        {
            Serial.println(" | send FAILED");
        }

        delay(PACKET_GAP_MS);
    }

    Serial.printf("--- Summary %d dBm | sent %d/%d (failed %d)", power, sent, PACKETS_PER_LEVEL,
                  PACKETS_PER_LEVEL - sent);
    if (sent > 0)
    {
        Serial.printf(" | avg airtime %.1f ms", airtimeSum / sent);
    }
    Serial.println(" ---");
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    Serial.println("LoRa power_tx: TX");

    if (!LoRaIOT_setup(loraConfig))
    {
        while (1)
            ;
    }
}

void loop()
{
    sweep++;
    for (int i = 0; i < NUM_POWERS; i++)
    {
        runLevel(TX_POWERS[i]);
        delay(LEVEL_GAP_MS);
    }
}
