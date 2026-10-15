#pragma once
#ifndef LORA_IOT_H
#define LORA_IOT_H

#include <Arduino.h>
#include "LoRaConfig.h"
#include "LoRa_IOT.h"

/**
 * @brief Set to true by the DIO0 interrupt when a packet arrives.
 *        The application must reset it after handling the packet.
 */
extern volatile bool rxFlag;

/**
 * @brief Initializes the LoRa module with the given configuration.
 * @return true if the module responds.
 */
bool LoRaIOT_setup(const LoRaConfig &config);

/**
 * @brief Sends a message over LoRa.
 * @return true if the message was sent successfully.
 */
bool LoRaIOT_send(const String &message);

/**
 * @brief Checks whether a packet has arrived and, if so, stores it in message.
 *        This function should be called continuously from loop().
 * @return true if a packet has been received.
 */
bool LoRaIOT_receive(String &message);

#endif // LORA_IOT_Hs