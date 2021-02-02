#include <ledmatrix.h>

 
// Member functions definitions including constructor
LED_Matrix::LED_Matrix(int rows, int cols, Adafruit_NeoPixel& leds) // constructor
{
  number_of_cols = cols;
  number_of_rows = rows;
  number_of_pixels = cols * rows; 
  _leds = leds;
  _doc = new DynamicJsonDocument(1024);
}
void LED_Matrix::handle_mqtt_message(String &topic, String &payload, MQTTClient &client)
{
    if (topic.lastIndexOf("/setpixel_rgb/") >= 0)
    {
        // Serial.println("PAYLOAD: "+ payload);
        // validate payload 
        // must be 6 chars ( 2 red, 2 green, 2 blue )
        if ( payload.length() == 6 )
        {
            unsigned long color = strtoul( payload.c_str(), nullptr, 16);
            Serial.println(color);
            //int color = payload.toInt();
            int index = topic.lastIndexOf("/");
            // setting the color auf of a single pixel
            // extract the pixel number from the topic
            // and validate it 
            int pixel = topic.substring( index + 1 ).toInt();
            if (pixel >= 0 && pixel < number_of_pixels) 
            {
            _leds.setPixelColor(pixel, color); // Set pixel 'c' to value 'color'
            _leds.show();
            }      
        }
    }
    else if (topic.lastIndexOf("/setpixel_hsv/") >= 0)
    {
        // Serial.println("PAYLOAD: "+ payload);
        unsigned long temp_val = strtoul( payload.c_str(), nullptr, 16);
        uint16_t hue = 0;
        unsigned char sat = 0;
        unsigned char val = 0;
        
        if ((payload.length() == 6) || (payload.length() == 8))
        {
            hue = (temp_val >> 16 ) & 0xFFFF;
            if (payload.length() == 6) hue *= 256;
            sat = ((temp_val % 65536 ) >> 8) & 0xFF;
            val = (temp_val % 65536 ) & 0xFF;
            // Serial.println("HUE " + String(hue));
            // Serial.println("SAT " + String(sat));
            // Serial.println("VAL " + String(val));
        }
        uint32_t rgbcolor = _leds.gamma32(_leds.ColorHSV(hue, sat, val));
        
        //int color = payload.toInt();
        int index = topic.lastIndexOf("/");
        // setting the color auf of a single pixel
        // extract the pixel number from the topic
        // and validate it 
        int pixel = topic.substring( index + 1 ).toInt();
        // Serial.println("#" + String(pixel) + " RGB: " + String(rgbcolor));
        if (pixel >= 0 && pixel < number_of_pixels) 
        {
            _leds.setPixelColor(pixel, rgbcolor); // Set pixel 'c' to value 'color'
            _leds.show();
        }      
    }
    else if (topic.lastIndexOf("/fillpixel_hsv/") >= 0)
    {
        // Serial.println("PAYLOAD: "+ payload);
        unsigned long temp_val = strtoul( payload.c_str(), nullptr, 16);
        uint16_t hue = 0;
        unsigned char sat = 0;
        unsigned char val = 0;
        
        if ((payload.length() == 6) || (payload.length() == 8))
        {
            hue = (temp_val >> 16 ) & 0xFFFF;
            if (payload.length() == 6) hue *= 256;
            sat = ((temp_val % 65536 ) >> 8) & 0xFF;
            val = (temp_val % 65536 ) & 0xFF;
            // Serial.println("HUE " + String(hue));
            // Serial.println("SAT " + String(sat));
            // Serial.println("VAL " + String(val));
        }
        uint32_t rgbcolor = _leds.gamma32(_leds.ColorHSV(hue, sat, val));
        // Serial.println("RGB: " + String(rgbcolor));
        //int color = payload.toInt();
        int index = topic.lastIndexOf("/");
        // setting the color auf of a single pixel
        // extract the pixel number from the topic
        // and validate it 
        String fill = topic.substring( index + 1 );
        int commaindex = fill.lastIndexOf(",");
        int count = fill.substring(commaindex + 1).toInt();
        int start = fill.substring(0,commaindex).toInt();
        Serial.println(String(start) + " " +String(count));
        //if (pixel >= 0 && pixel < number_of_pixels) 
        for (int pixel = start; pixel < number_of_pixels; pixel++)
        {
            Serial.println(String(pixel));
            _leds.setPixelColor(pixel, rgbcolor); // Set pixel 'c' to value 'color'           
            count--;
            if (!count) break;
        }      
        _leds.show();
    }
    else if (topic.lastIndexOf("/fill_hsv") >= 0)
    {
        // Serial.println("PAYLOAD: "+ payload);
        unsigned long temp_val = strtoul( payload.c_str(), nullptr, 16);
        uint16_t hue = 0;
        unsigned char sat = 0;
        unsigned char val = 0;
        
        if ((payload.length() == 6) || (payload.length() == 8))
        {
            hue = (temp_val >> 16 ) & 0xFFFF;
            if (payload.length() == 6) hue *= 256;
            sat = ((temp_val % 65536 ) >> 8) & 0xFF;
            val = (temp_val % 65536 ) & 0xFF;
        }
        uint32_t rgbcolor = _leds.gamma32(_leds.ColorHSV(hue, sat, val));
        for (int pixel = 0; pixel < number_of_pixels; pixel++)
        {
            // Serial.println(String(pixel));
            _leds.setPixelColor(pixel, rgbcolor); // Set pixel 'c' to value 'color'           
        }      
        _leds.show();
    }
    else if (topic.lastIndexOf("/json") >= 0)
    {
        DynamicJsonDocument doc(8096);
        Serial.println("JSON:" + payload);
        DeserializationError error = deserializeJson(doc, payload); 
        if (!error)  
        {           
            if (doc.containsKey("test"))
            {
                const char* item = doc["test"];
                Serial.println("DESERIALIZED:" + String(item));
            }
            else
                Serial.println("KEY test do not exist");

        } 
    }
    else if (topic.lastIndexOf("/clear") >= 0)
    {
        _leds.clear();
        _leds.show();
    }
    // String _topic    = topic_root + "rep/" + String(accessnumber); 
}
    
void LED_Matrix::begin( void )
{
    _leds.begin();
    _leds.setBrightness(30);
    _leds.setPixelColor(0, 20000); // Set pixel 'c' to value 'color'
    _leds.show();
}