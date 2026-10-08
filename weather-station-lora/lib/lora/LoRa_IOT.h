#pragma once
#ifndef LORA_IOT_H
#define LORA_IOT_H

#include <Arduino.h>
#include "LoRaConfig.h"

/**
 * @brief  Inicializa el módulo LoRa con la configuración dada.
 * @return true si el módulo respondió.
 */
bool LoRaIOT_setup(const LoRaConfig &config);

/**
 * @brief  Envía un mensaje por LoRa.
 * @return true si se envió correctamente.
 */
bool LoRaIOT_send(const String &message);

/**
 * @brief  Comprueba si ha llegado un paquete y, si es así, lo guarda en message.
 *         Hay que llamarla continuamente desde loop().
 * @return true si se ha recibido un paquete.
 */
bool LoRaIOT_receive(String &message);

#endif // LORA_IOT_H
