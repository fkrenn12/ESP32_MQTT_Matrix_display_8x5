#include <ledmatrix.h>
 
// Member functions definitions including constructor
// ---------------------------------------------------------------------------------------------------------------------
LED_Matrix::LED_Matrix(int rows, int cols, Adafruit_NeoPixel& leds) // constructor
// ---------------------------------------------------------------------------------------------------------------------
{
  number_of_cols = cols;
  number_of_rows = rows;
  number_of_pixels = cols * rows; 
  _leds = leds;
  _doc = new DynamicJsonDocument(1024);
}

// ---------------------------------------------------------------------------------------------------------------------
bool LED_Matrix::is_valid_ascii_rgb(String ascii_rgb)
// ---------------------------------------------------------------------------------------------------------------------
{
    bool valid = false;
    // check chars
    ascii_rgb.toLowerCase();
    // TODO: checking valid asii chars
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
    // TODO: checking valid asii chars
    // check length
    if ((ascii_hsv.length() == 6 ) || (ascii_hsv.length() == 8 ))  valid = true;
    return(valid);
}

// ---------------------------------------------------------------------------------------------------------------------
uint32_t LED_Matrix::ascii_to_rgb(String ascii_rgb)
// ---------------------------------------------------------------------------------------------------------------------
{
    unsigned long ul_rgb = 0;
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
    unsigned long temp_val = strtoul( ascii_hsv.c_str(), nullptr, 16);
    uint16_t hue = 0;
    unsigned char sat = 0;
    unsigned char val = 0;

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
    if (topic.lastIndexOf("/setpixel_rgb/") >= 0)
    // ------------------------------------------
    {     
        if (!is_valid_ascii_rgb(payload)) return;
        int index = topic.lastIndexOf("/");
        int pixel = topic.substring( index + 1 ).toInt();
        if (pixel >= 0 && pixel < number_of_pixels) 
        {
            _leds.setPixelColor(pixel, ascii_to_rgb(payload));
        }      
    }
    // -----------------------------------------------
    else if (topic.lastIndexOf("/setpixel_hsv/") >= 0)
    // -----------------------------------------------
    {   
        if (!is_valid_ascii_hsv(payload)) return;
        int index = topic.lastIndexOf("/");
        int pixel = topic.substring( index + 1 ).toInt();
        if (pixel >= 0 && pixel < number_of_pixels) 
        {
            _leds.setPixelColor(pixel, ascii_hsv_to_rgb(payload)); 
        }      
    }
    // ------------------------------------------------
    else if (topic.lastIndexOf("/fillpixel_hsv/") >= 0)
    // ------------------------------------------------
    {
        if (!is_valid_ascii_hsv(payload)) return;
        int index = topic.lastIndexOf("/");
        String fill = topic.substring( index + 1 );
        int commaindex = fill.lastIndexOf(",");
        int count = fill.substring(commaindex + 1).toInt();
        int start = fill.substring(0,commaindex).toInt();
        Serial.println(String(start) + " " +String(count));
        for (int pixel = start; pixel < number_of_pixels; pixel++)
        {
            Serial.println(String(pixel));
            _leds.setPixelColor(pixel, ascii_hsv_to_rgb(payload));         
            count--;
            if (!count) break;
        }      
    }
    // ------------------------------------------
    else if (topic.lastIndexOf("/fill_hsv") >= 0)
    // ------------------------------------------
    {
        if (!is_valid_ascii_hsv(payload)) return;
        for (int pixel = 0; pixel < number_of_pixels; pixel++)
        {
            _leds.setPixelColor(pixel, ascii_hsv_to_rgb(payload));           
        }      
    }
    // --------------------------------------
    else if (topic.lastIndexOf("/json") >= 0)
    // --------------------------------------
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
    // ---------------------------------------
    else if (topic.lastIndexOf("/clear") >= 0)
    // ---------------------------------------
    {
        _leds.clear();
    }

    if (show) _leds.show();
}
    
// ---------------------------------------------------------------------------------------------------------------------    
void LED_Matrix::begin( void )
// ---------------------------------------------------------------------------------------------------------------------
{
    _leds.begin();
    _leds.setBrightness(30);
    _leds.setPixelColor(0, 20000); // Set pixel 'c' to value 'color'
    _leds.show();
}