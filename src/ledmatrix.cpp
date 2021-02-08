#include <ledmatrix.h>
 
// Member functions definitions including constructor
// ---------------------------------------------------------------------------------------------------------------------
LED_Matrix::LED_Matrix(uint32_t rows, uint32_t cols, Adafruit_NeoPixel& leds) // constructor
// ---------------------------------------------------------------------------------------------------------------------
{
  number_of_cols = cols;
  number_of_rows = rows;
  number_of_pixels = cols * rows; 
  _leds = leds;
}

// ---------------------------------------------------------------------------------------------------------------------
uint32_t LED_Matrix::transpose_pixel(uint32_t pixel)
// ---------------------------------------------------------------------------------------------------------------------
{
    return (pixel);
}

// ---------------------------------------------------------------------------------------------------------------------
void LED_Matrix::set_pixel_color(uint32_t start, uint32_t  count, uint32_t color)
// ---------------------------------------------------------------------------------------------------------------------
{
    // Serial.println("SET_PIXEL_COLOR: " + String(start) + " " + String(count) + " " + String(color));
    switch (count)
    {
        case 0: return; 
        case 1: _leds.setPixelColor(transpose_pixel(start), color); break;
        default:
        {
            // Serial.println("default:");
            for (int pixel = start; pixel < number_of_pixels; pixel++)
            {
                // Serial.println(String(pixel) + " " + String(color));
                _leds.setPixelColor(transpose_pixel(pixel), color);         
                count--;
                if (!count) break;
            } 
        }break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
bool LED_Matrix::is_valid_ascii_rgb(String ascii_rgb)
// ---------------------------------------------------------------------------------------------------------------------
{
    bool valid = false;
    // check chars
    ascii_rgb.toLowerCase();
    // TODO: checking valid ascii chars
    // check length
    if (ascii_rgb.length() == 6 ) valid = true;
    return(valid);
}

// ---------------------------------------------------------------------------------------------------------------------
bool LED_Matrix::is_valid_ascii_hsv(String ascii_hsv)
// ---------------------------------------------------------------------------------------------------------------------
{
    bool valid = false;
    // check chars
    ascii_hsv.toLowerCase();
    // TODO: checking valid ascii chars
    // check length
    if ((ascii_hsv.length() == 6 ) || (ascii_hsv.length() == 8 ))  valid = true;
    return(valid);
}

// ---------------------------------------------------------------------------------------------------------------------
uint32_t LED_Matrix::ascii_to_rgb(String ascii_rgb)
// ---------------------------------------------------------------------------------------------------------------------
{
    uint32_t  ul_rgb = 0;
    if (ascii_rgb.length() == 6 )
    {
        ul_rgb = strtoul( ascii_rgb.c_str(), nullptr, 16);
    }
    return(ul_rgb);
}

// ---------------------------------------------------------------------------------------------------------------------
uint32_t LED_Matrix::ascii_hsv_to_rgb(String ascii_hsv)
// ---------------------------------------------------------------------------------------------------------------------
{
    uint32_t temp_val = strtoul( ascii_hsv.c_str(), nullptr, 16);
    uint16_t hue = 0;
    uint8_t  sat = 0;
    uint8_t  val = 0;

    if ((ascii_hsv.length() == 6) || (ascii_hsv.length() == 8))
    {
        hue = (temp_val >> 16 ) & 0xFFFF;
        if (ascii_hsv.length() == 6) hue *= 256;
        sat = ((temp_val % 65536 ) >> 8) & 0xFF;
        val = (temp_val % 65536 ) & 0xFF;
        //Serial.println("HUE " + String(hue));Serial.println("SAT " + String(sat));Serial.println("VAL " + String(val));
    }
    return (_leds.gamma32(_leds.ColorHSV(hue, sat, val)));
}

// ---------------------------------------------------------------------------------------------------------------------
void LED_Matrix::handle_mqtt_message(String topic, String payload, MQTTClient &client)
// ---------------------------------------------------------------------------------------------------------------------
{
    // handle the star character "*"
    // * means that the show command will be executed at least
    bool show = (topic.lastIndexOf("*") >= 0);
    // we remove the * for easier following parsing
    if (show) topic.replace("*","");

     // ------------------------------------------
    if (topic.lastIndexOf("/bright") >= 0)
    // ------------------------------------------
    {     
        uint32_t brightness = payload.toInt();
        if ((brightness >= 0) && (brightness <= 100))
            _leds.setBrightness(brightness);    
    }  
    // -----------------------------------------------------------------------------------------
    else if ((topic.lastIndexOf("/pixel_rgb/") >= 0) || (topic.lastIndexOf("/pixel_hsv") >= 0))
    // -----------------------------------------------------------------------------------------
    {  
        uint32_t  start = 0;
        int32_t   count = 0; 
        bool is_hsv = (topic.lastIndexOf("hsv") >= 0);
        uint32_t item_len = 6;
        if (topic.lastIndexOf("hsv16") >= 0) item_len = 8;

        String pixelrange = topic.substring( topic.lastIndexOf("/") + 1 );
        int seperator_index = pixelrange.lastIndexOf(CHAR_PIXELSTART_COUNT_SEPERATOR);
        if (seperator_index > 0)
        {
            // seperator detected - this is a series of pixels
            int32_t stop    = INT32_MAX;
            start           = pixelrange.substring(0,seperator_index).toInt();
            String str_stop = pixelrange.substring(seperator_index + 1);
            // Serial.println("STR:"+String(str_stop)+":");
            if (str_stop.length() > 0)
                stop        = str_stop.toInt();
            // Serial.println("INT:"+String(stop)+":");
            if ( stop > start) 
                count       = stop - start; //pixelrange.substring(seperator_index + 1).toInt();
            // Serial.println("CNT:"+String(count)+":");
        }
        else
        {
            // seperator not detected - this is a single pixel
            start = pixelrange.toInt();
            count = 1;
        }
        if (count != 0)
        {
            // uint32_t item_len = payload.length()/count;
            // uint32_t item_len = 6;
            // Serial.println("ITEM_LEN " + String(item_len));
            // uint32_t stop = count;
            // is it a valid payload?
            // bool valid = true; // (payload.length()%count == 0); // must be divided without rest
            // if (is_hsv) valid = valid && ((item_len == 6) || (item_len  == 8)); // 6 or 8 at hsv
            // else        valid = valid && (item_len == 6); // 6 only at rgb
            // Serial.println("VALID " + String(valid) + "is_hsv: " + String(is_hsv));
            
            //if (valid)
            {      
                int pos = 0;
                // iterating through payload string and pick up substrings
                for (int pixel = start; pixel < number_of_pixels; pixel++)
                {
                    // pos = (stop - count) * item_len;
                    String item = payload.substring(pos, pos + item_len);
                    if (item.length() == 0 ) break;
                    if (item.startsWith("_") || item.startsWith("-") || item.startsWith("."))
                    {
                        pos++;
                        continue;
                    }
                    if (is_hsv)
                        set_pixel_color(pixel,1,ascii_hsv_to_rgb(payload.substring(pos, pos + item_len))); 
                    else
                        set_pixel_color(pixel,1,ascii_to_rgb(payload.substring(pos, pos + item_len))); 
                    pos += item_len;
                    if (--count <= 0) break;
                }       
            }
        }
    }
    // ------------------------------------------------
    else if (topic.lastIndexOf("/fillpixel_hsv/") >= 0)
    // ------------------------------------------------
    {
        uint32_t  count = 0;
        uint32_t  start = 0;
        if (!is_valid_ascii_hsv(payload)) return;
        int  index = topic.lastIndexOf("/");
        String fill = topic.substring( index + 1 );
        int  seperator_index = fill.lastIndexOf(CHAR_PIXELSTART_COUNT_SEPERATOR);
        if (seperator_index > 0)
        {
            count = fill.substring(seperator_index  + 1).toInt();
            start = fill.substring(0, seperator_index).toInt();
        }
        else 
        {
            count = 1;
            start = fill.substring(0).toInt();
        }
        set_pixel_color(start, count, ascii_hsv_to_rgb(payload));   
    }
    // ------------------------------------------
    else if (topic.lastIndexOf("/filldisplay_hsv") >= 0)
    // ------------------------------------------
    {
        if (!is_valid_ascii_hsv(payload)) return;
        set_pixel_color(0, number_of_pixels, ascii_hsv_to_rgb(payload));     
    }
    /*
    // --------------------------------------
    else if (topic.lastIndexOf("/json") >= 0)
    // --------------------------------------
    {
        DynamicJsonDocument doc(8096);
        Serial.println("JSON:" + payload);
        DeserializationError error = deserializeJson(doc, payload); 
        if (!error)  
        {           
            if (doc.containsKey("bright"))
            {
                const char* item = doc["bright"];
                Serial.println("DESERIALIZED:" + String(item));
            }
            else if (doc.containsKey("pixel_hsv"))
            {
                const char* item = doc["pixel_hsv"];               
                Serial.println("DESERIALIZED:" + String(item));
            }
            else
                Serial.println("KEY test do not exist");

        } 
    }
    */
    // ---------------------------------------
    else if (topic.lastIndexOf("/clear") >= 0)
    // ---------------------------------------
    {
        _leds.clear();
    }
    // ---------------------------------------
    else if (topic.lastIndexOf("/show") >= 0)
    // ---------------------------------------
    {
        _leds.show();
        return;
    }

    if (show) _leds.show();
}
    
// ---------------------------------------------------------------------------------------------------------------------    
void LED_Matrix::begin( void )
// ---------------------------------------------------------------------------------------------------------------------
{
    _leds.begin();
    _leds.setBrightness(50);
    set_pixel_color(0,1, ascii_hsv_to_rgb("ffffff")); 
    _leds.show();
    delay(1000);
    _leds.clear();
    _leds.show();
}