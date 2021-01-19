#ifndef _CONNECTING_H
#define _CONNECTING_H

#include <WiFi.h>
#include <PubSubClient.h> // MQTT Library

void connectTask( void * pvParameters );

#endif