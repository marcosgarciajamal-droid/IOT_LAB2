/**
 * Experiment: range-speed-balance-bw
 * "Adjusting the bandwidth and data rate to balance transmission speed and range."
 *
 * The same file is built for both boards (MODE=1 -> TX, MODE=0 -> RX).
 *
 * Both boards sync their clock through WiFi/NTP and then sweep the
 * EXPERIMENT_CONFIGS table in lockstep: every config owns a time window
 * aligned to the wall clock, so TX and RX switch at the same time without any
 * handshake.
 *
 * EU868 duty cycle: 868.0-868.6 MHz (sub-band g1) allows the transmitter to be
 * on at most 1% of the time. After a packet of T ms the channel must stay
 * silent for 99*T ms, so the send interval (and therefore the window length)
 * of every config is derived from its time-on-air.
 *
 * TX sends {"id":N,"t":"HH:MM:SS.mmm"} at fixed slots inside each window and
 * prints the measured time-on-air (duration of endPacket) and the theoretical
 * one (Semtech formula).
 * RX prints RSSI, SNR, its own timestamp and t_rx - t_tx, which should match
 * the time-on-air (plus the NTP sync error, typically a few ms).
 *
 *   pio run -e range_speed_bw_tx -t upload
 *   pio run -e range_speed_bw_rx -t upload
 */

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <sys/time.h>
#include <time.h>
#include <config.h>
#include "LoRaConfig.h"
#include "LoRa_IOT.h"
#include "wifi_manager.hh"

// =============================
// === EXPERIMENT PARAMETERS ===
// =============================

#define NTP_SERVER "pool.ntp.org"
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3" // Europe/Madrid
#define NTP_SYNC_TIMEOUT_MS 20000

#define DUTY_CYCLE 0.01f          // EU868 sub-band g1: 1%
#define PACKETS_PER_CONFIG 10     // packets sent in every window
#define MIN_SEND_INTERVAL_MS 5000 // floor for fast configs, keeps the log readable
#define GUARD_MS 3000             // silence at the start of a window

struct ExperimentConfig
{
    long bandwidth; // Hz
    uint8_t sf;     // spreading factor [6-12]
};

// BW {125, 250, 500} kHz x SF {7, 9, 12}, CR 4/5 fixed
const ExperimentConfig EXPERIMENT_CONFIGS[] = {
    {125000L, 7},
    {125000L, 9},
    {125000L, 12},
    {250000L, 7},
    {250000L, 9},
    {250000L, 12},
    {500000L, 7},
    {500000L, 9},
    {500000L, 12},
};
const uint8_t NUM_CONFIGS = sizeof(EXPERIMENT_CONFIGS) / sizeof(EXPERIMENT_CONFIGS[0]);

volatile bool rxFlag = false;

WiFiManager wifiManager(WIFI_SSID, WIFI_PASSWORD, WIFI_TIMEOUT_MS, WIFI_RETRY_MS);
LoRaConfig loraConfig;

// Schedule, filled by computeSchedule()
long sendIntervalMs[NUM_CONFIGS];
long windowLengthMs[NUM_CONFIGS];
long cycleLengthMs = 0;

int currentConfig = -1;
int64_t currentWindowStart = -1;
int firstSlot = 0;

// ============
// === TIME ===
// ============

int64_t nowEpochMs()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/** Formats an epoch in ms as local "HH:MM:SS.mmm". */
String formatTime(int64_t epochMs)
{
    time_t seconds = epochMs / 1000;
    struct tm timeInfo;
    localtime_r(&seconds, &timeInfo);

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d",
             timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, (int)(epochMs % 1000));
    return String(buffer);
}

/** Parses "HH:MM:SS.mmm" into ms since midnight, -1 if malformed. */
long parseTimeOfDayMs(const char *text)
{
    int hours, minutes, seconds, millis;
    if (sscanf(text, "%d:%d:%d.%d", &hours, &minutes, &seconds, &millis) != 4)
    {
        return -1;
    }
    return ((hours * 60L + minutes) * 60L + seconds) * 1000L + millis;
}

/** Local time of day in ms since midnight. */
long timeOfDayMs(int64_t epochMs)
{
    time_t seconds = epochMs / 1000;
    struct tm timeInfo;
    localtime_r(&seconds, &timeInfo);
    return ((timeInfo.tm_hour * 60L + timeInfo.tm_min) * 60L + timeInfo.tm_sec) * 1000L + (long)(epochMs % 1000);
}

/** Connects to WiFi and blocks until the clock is set by NTP. */
bool syncClock()
{
    wifiManager.begin();
    while (!wifiManager.isConnected())
    {
        wifiManager.update();
        delay(10);
    }

    configTzTime(TIME_ZONE, NTP_SERVER);

    unsigned long start = millis();
    while (time(nullptr) < 1700000000) // still 1970 -> not synced yet
    {
        if (millis() - start > NTP_SYNC_TIMEOUT_MS)
        {
            return false;
        }
        delay(100);
    }

    Serial.print("Clock synced: ");
    Serial.println(formatTime(nowEpochMs()));
    return true;
}

// ==================
// === LORA MATHS ===
// ==================

/**
 * Theoretical time-on-air in ms (Semtech SX1276 datasheet, section 4.1.1.7).
 * Explicit header and CRC on, as set by LoRaIOT_setup().
 */
float airtimeMs(const ExperimentConfig &config, size_t payloadLength)
{
    float symbolMs = (float)(1L << config.sf) / config.bandwidth * 1000.0f;
    int lowDataRate = symbolMs > 16.0f ? 1 : 0; // same rule as LoRa.setLdoFlag()
    int crc = 1;
    int implicitHeader = 0;

    float preambleMs = (loraConfig.preamble_len + 4.25f) * symbolMs;

    float numerator = 8.0f * payloadLength - 4.0f * config.sf + 28 + 16 * crc - 20 * implicitHeader;
    float denominator = 4.0f * (config.sf - 2 * lowDataRate);
    float payloadSymbols = 8 + max(ceilf(numerator / denominator) * (loraConfig.cr + 4), 0.0f);

    return preambleMs + payloadSymbols * symbolMs;
}

/** JSON payload sent by TX; "t" is the TX timestamp. */
String buildPayload(int id, const String &timestamp)
{
    return "{\"id\":" + String(id) + ",\"t\":\"" + timestamp + "\"}";
}

// ================
// === SCHEDULE ===
// ================

/**
 * Derives the send interval of every config from the 1% duty cycle
 * (interval >= airtime / DUTY_CYCLE) and prints the resulting schedule.
 */
void computeSchedule()
{
    // Longest payload of a window: highest id, timestamp has a fixed length
    size_t payloadLength = buildPayload(PACKETS_PER_CONFIG - 1, "00:00:00.000").length();

    Serial.println();
    Serial.printf("Schedule (payload %u bytes, duty cycle %.1f%%):\n",
                  (unsigned)payloadLength, DUTY_CYCLE * 100);

    cycleLengthMs = 0;
    for (int i = 0; i < NUM_CONFIGS; i++)
    {
        float airtime = airtimeMs(EXPERIMENT_CONFIGS[i], payloadLength);
        sendIntervalMs[i] = max((long)ceilf(airtime / DUTY_CYCLE), (long)MIN_SEND_INTERVAL_MS);
        windowLengthMs[i] = GUARD_MS + PACKETS_PER_CONFIG * sendIntervalMs[i];
        cycleLengthMs += windowLengthMs[i];

        Serial.printf("  %d | BW %5.1f kHz | SF %2d | airtime %7.1f ms | interval %6.1f s | window %6.1f s\n",
                      i + 1, EXPERIMENT_CONFIGS[i].bandwidth / 1000.0f, EXPERIMENT_CONFIGS[i].sf,
                      airtime, sendIntervalMs[i] / 1000.0f, windowLengthMs[i] / 1000.0f);
    }

    Serial.printf("Full cycle: %.1f min\n", cycleLengthMs / 60000.0f);
}

/** Offset of slot k from the start of the window of config index. */
long slotOffsetMs(int index, int k)
{
    return GUARD_MS + (long)k * sendIntervalMs[index];
}

/** First slot still ahead when a window is entered elapsedMs after its start. */
int firstSlotAfter(int index, long elapsedMs)
{
    if (elapsedMs <= GUARD_MS)
    {
        return 0;
    }
    long interval = sendIntervalMs[index];
    return (elapsedMs - GUARD_MS + interval - 1) / interval;
}

/**
 * Finds the window containing nowMs. Cycles are aligned to the epoch, so
 * both boards get the same answer from the same clock.
 */
void locateWindow(int64_t nowMs, int &index, int64_t &windowStart)
{
    windowStart = nowMs - nowMs % cycleLengthMs;
    for (index = 0; index < NUM_CONFIGS - 1; index++)
    {
        if (nowMs < windowStart + windowLengthMs[index])
        {
            return;
        }
        windowStart += windowLengthMs[index];
    }
}

// ========================
// === CONFIG SWITCHING ===
// ========================

void applyConfig(int index)
{
    const ExperimentConfig &config = EXPERIMENT_CONFIGS[index];

    LoRa.idle();
    LoRa.setSignalBandwidth(config.bandwidth);
    LoRa.setSpreadingFactor(config.sf); // also updates the LDO flag

    Serial.println();
    Serial.printf("=== Config %d/%d | BW %.1f kHz | SF %d | CR 4/%d | interval %.1f s | %d packets ===\n",
                  index + 1, NUM_CONFIGS, config.bandwidth / 1000.0f, config.sf, loraConfig.cr + 4,
                  sendIntervalMs[index] / 1000.0f, PACKETS_PER_CONFIG - firstSlot);
}

/**
 * Switches config when a new window starts.
 * @return true if the window changed.
 */
bool updateWindow(int64_t nowMs)
{
    int index;
    int64_t windowStart;
    locateWindow(nowMs, index, windowStart);
    if (windowStart == currentWindowStart)
    {
        return false;
    }

    currentWindowStart = windowStart;
    currentConfig = index;
    // Booting mid-window: skip the slots already gone instead of sending
    // them back-to-back, which would break the duty cycle
    firstSlot = firstSlotAfter(index, nowMs - windowStart);
    applyConfig(index);
    return true;
}

// ==========
// === TX ===
// ==========
#if MODE == 1

int nextSlot = 0;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    Serial.println("LoRa range-speed-balance-bw: TX");

    if (!syncClock())
    {
        Serial.println("NTP sync failed");
        while (1)
            ;
    }

    if (!LoRaIOT_setup(loraConfig))
    {
        while (1)
            ;
    }

    computeSchedule();
}

void loop()
{
    int64_t now = nowEpochMs();

    if (updateWindow(now))
    {
        nextSlot = firstSlot;
    }

    if (nextSlot >= PACKETS_PER_CONFIG)
    {
        return;
    } // window done, stay silent
    if (now < currentWindowStart + slotOffsetMs(currentConfig, nextSlot))
    {
        return;
    }

    int64_t sentAt = nowEpochMs();
    String timestamp = formatTime(sentAt);
    String message = buildPayload(nextSlot, timestamp);

    unsigned long start = micros();
    bool ok = LoRaIOT_send(message); // blocks until TxDone
    float measuredMs = (micros() - start) / 1000.0f;

    Serial.print("[TX|" + timestamp + "] " + message);
    if (ok)
    {
        Serial.printf(" | airtime %.1f ms (theory %.1f ms)\n",
                      measuredMs, airtimeMs(EXPERIMENT_CONFIGS[currentConfig], message.length()));
    }
    else
    {
        Serial.println(" | send FAILED");
    }

    nextSlot++;
}

// ==========
// === RX ===
// ==========
#else

int receivedCount = 0;
float rssiSum = 0;
float snrSum = 0;
float deltaSum = 0;

void printWindowSummary()
{
    if (currentConfig < 0)
    {
        return;
    }

    const ExperimentConfig &config = EXPERIMENT_CONFIGS[currentConfig];
    int expected = PACKETS_PER_CONFIG - firstSlot;
    float lossPercent = expected > 0 ? 100.0f * (expected - receivedCount) / expected : 0.0f;

    Serial.printf("--- Summary BW %.1f kHz SF %d | received %d/%d (loss %.0f%%)",
                  config.bandwidth / 1000.0f, config.sf, receivedCount, expected, lossPercent);
    if (receivedCount > 0)
    {
        Serial.printf(" | avg RSSI %.1f dBm | avg SNR %.1f dB | avg delta %.1f ms",
                      rssiSum / receivedCount, snrSum / receivedCount, deltaSum / receivedCount);
    }
    Serial.println(" ---");

    receivedCount = 0;
    rssiSum = 0;
    snrSum = 0;
    deltaSum = 0;
}

void handlePacket(int64_t receivedAt)
{
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0)
    {
        Serial.println("[RX] interrupt without valid packet (CRC error?)");
        return;
    }

    String message = "";
    while (LoRa.available())
    {
        message += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    String timestamp = formatTime(receivedAt);

    Serial.printf("[RX|%ddBm|%s] %s | SNR %.1f dB", rssi, timestamp.c_str(), message.c_str(), snr);

    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, message))
    {
        Serial.println(" | JSON parsing failed");
        return;
    }

    long sentOfDay = parseTimeOfDayMs(doc["t"] | "");
    if (sentOfDay < 0)
    {
        Serial.println(" | bad timestamp");
        return;
    }

    long delta = timeOfDayMs(receivedAt) - sentOfDay;
    if (delta < -43200000L)
    {
        delta += 86400000L;
    } // midnight wrap

    float theory = airtimeMs(EXPERIMENT_CONFIGS[currentConfig], packetSize);
    Serial.printf(" | delta %ld ms (theory %.1f ms)\n", delta, theory);

    receivedCount++;
    rssiSum += rssi;
    snrSum += snr;
    deltaSum += delta;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    Serial.println("LoRa range-speed-balance-bw: RX");

    if (!syncClock())
    {
        Serial.println("NTP sync failed");
        while (1)
            ;
    }

    if (!LoRaIOT_setup(loraConfig))
    {
        while (1)
            ;
    }

    computeSchedule();
}

void loop()
{
    if (rxFlag)
    {
        // Timestamp first, before any SPI access
        int64_t receivedAt = nowEpochMs();
        rxFlag = false;

        handlePacket(receivedAt);
        LoRa.receive();
    }

    // Windows change while TX is silent (the last packet starts a full
    // interval before the window end), never in the middle of a packet
    int64_t now = nowEpochMs();
    if (currentWindowStart < 0 || now >= currentWindowStart + windowLengthMs[currentConfig])
    {
        printWindowSummary();
        updateWindow(now);
        rxFlag = false;
        LoRa.receive();
    }
}

#endif
