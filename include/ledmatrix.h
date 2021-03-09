#ifndef _LEDMATRIX_H
#define _LEDMATRIX_H

#include <Arduino.h>
#include <MQTT.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <wifi_mqtt.h>

#define CHAR_PIXELSTART_COUNT_SEPERATOR ":"

class LED_Matrix 
{
   private:
      uint32_t number_of_rows;
      uint32_t number_of_cols;
      uint32_t number_of_pixels;
      uint32_t ascii_hsv_to_rgb(String hsv);
      uint32_t ascii_to_rgb(String ascii_rgb);
      bool is_valid_ascii_rgb(String ascii_rgb);
      bool is_valid_ascii_hsv(String ascii_hsv);
      uint32_t transpose_pixel(uint32_t pixel);
      void set_pixel_color(uint32_t start, uint32_t count, uint32_t color);
      Adafruit_NeoPixel _leds; 
   public:
      void handle_mqtt_message(WifiMQTT* mqtt, String topic, String payload);
      LED_Matrix(uint32_t,uint32_t,Adafruit_NeoPixel&);  // Matrixconstructor 
      void begin(void);
};
#endif