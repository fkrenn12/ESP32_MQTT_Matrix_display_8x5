#ifndef _LEDMATRIX_H
#define _LEDMATRIX_H

#include <Arduino.h>
#include <MQTT.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

class LED_Matrix 
{
   private:
      int number_of_rows;
      int number_of_cols;
      int number_of_pixels;
      // Adafruit_NeoPixel& _leds;
      // 40 Pixels / Pin#18 @ ESP32-wroom / 2812Shield
      Adafruit_NeoPixel _leds; //(40, 18, NEO_GRB + NEO_KHZ800); // verified settings
      DynamicJsonDocument * _doc;
   public:
      void handle_mqtt_message(String &topic, String &payload, MQTTClient &client);
      LED_Matrix(int,int,Adafruit_NeoPixel&);  // Matrixconstructor 
      void begin(void);
};
#endif