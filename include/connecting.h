#ifndef _CONNECTING_H
#define _CONNECTING_H

#include <WiFi.h>
#include "TFT_eSPI.h"
#include <PubSubClient.h> // MQTT Library

void connectTask( void * pvParameters );

#endif