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
      uint32_t ascii_hsv_to_rgb(String hsv);
      uint32_t ascii_to_rgb(String ascii_rgb);
      bool is_valid_ascii_rgb(String ascii_rgb);
      bool is_valid_ascii_hsv(String ascii_hsv);
      Adafruit_NeoPixel _leds; 
      DynamicJsonDocument * _doc;
   public:
      void handle_mqtt_message(String topic, String payload, MQTTClient &client);
      LED_Matrix(int,int,Adafruit_NeoPixel&);  // Matrixconstructor 
      void begin(void);
};
#endif