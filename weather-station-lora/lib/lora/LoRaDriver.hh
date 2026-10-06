#pragma once

#include <LoRa.h>
#include <SPI.h>
#include "LoRaConfig.h"
#include <config.h>





bool beginLoRa(const LoRaConfig &config);
