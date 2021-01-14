#include <WiFi.h>
#include <PubSubClient.h> // MQTT Library
#include "SPI.h"
#include "TFT_eSPI.h"
#include "pubsubcontroller.h"
#include "connecting.h"
#include "listlib.h"
#include "analogmeter.h"
#include <EEPROM.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
#include "driver/uart.h"
// #include <iostream>

//#define DEBUG 1
//--------------------------------------------
#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"
#define BAUDRATE 250000

//--------------------------------------------
// Time Server setting
//--------------------------------------------
#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time
//--------------------------------------------
// ------------> Wifi settings <------------------
String  ssid            = "LAWIG14";        //Enter SSID
String  password        = "wiesengrund14";  //Enter Password
/*
const char  ssid[]      = "LAWIG14"; //Enter SSID
const char  password[]  = "wiesengrund14"; //Enter Password
*/
// --------> MQTT-Broker settings <---------------


String mqttIP    = "94.16.117.246"; 
String mqttUser  = "labor";
String mqttPass  = "labor"; 
/*
String mqttIP           = "91.132.147.143"; 
String mqttUser         = "franz";
String mqttPass         = "FK_s10rr6fr"; 
*/
int mqttPort            = 1883;
// -----------------------------------------
// global
// -----------------------------------------
boolean needReconnect = false;
boolean connected     = false;
portMUX_TYPE Mutex = portMUX_INITIALIZER_UNLOCKED;

// -----------------------------------------
// get a Unique ID ( we use the mac-address )
// -----------------------------------------
uint64_t chipid;
char chipid_str[13];    // 6 Bytes = 12 Chars + \0x00 = 13 Chars
//char topic[256];      // char buffer for topic used several times in code
List<String>SerialCommands(20); // max of 20 Items can be stored
WiFiClient            net;
PubSubClient          client(net);
TFT_eSPI              tft = TFT_eSPI();
PubSubController      PSC(client);//  = PubSubController(client);
Analogmeter           analog(tft);
TaskHandle_t          xHandle = NULL;
// prototypes
void messageReceived(char*, byte*, unsigned int);
void serialReceived(void);

//--------------------------------------------------
void serialcommands_handler(void)
//--------------------------------------------------
{
  int len = SerialCommands.Count();
  if (len>0)
  {
    for (int i = 0;i<len;i++)
    {
      String _cmd = SerialCommands[i];
      // sending it back ( development and testing only)
      // Serial.printf("#%s",_cmd.c_str());
      // if (i==len-1) Serial.print("\n\r");
      // else Serial.print(",");

      // first we look at the command selector
      // its the first character
      char maincommand = _cmd.charAt(0);
      // Serial.printf("+%s",_cmd.c_str());
      _cmd.remove(0,1);
      if (maincommand=='#')
      {
        // delete the first character
        // _cmd.remove(0,1);
        // lets extract the channel indicator
        int _next = _cmd.indexOf(":");
        if (_next<0) continue; // ":" is missing in command
        String _channel = _cmd.substring(0,_next);
        int _ichannel = _channel.toInt();
        if (_ichannel > CHANNEL_MAX) continue; // channel is too great
        int _start = _cmd.indexOf("[");
        int _end   = _cmd.indexOf("]");
        if ((_start<0)||(_end<0)) continue; // topic is missing
        String _topic = _cmd.substring(_start+1,_end);
        _next = _cmd.lastIndexOf(":");
        _cmd.remove(0,_next+1);  // remove chars which are already handled
        // the remaining cmd containes advanced settings
        // Serial.printf("Remaining: %s\n",_cmd.c_str());
        // Serial.printf("Channel:%i Topic: %s\n",_ichannel,_topic.c_str());
        // now, its time to store channel and topic into the list
        PSC.setChannelTopic(_ichannel,_topic,CHANNEL_MODE_LAST);
      }
      else if (maincommand=='%')
      {
          // display settings

      }
      else if (maincommand=='*')
      {
          // publishing commands ( power supplies, central measurement manager CMM )
      }
      else if (maincommand=='$')
      {
          // measuring and trigger settings
      }
      else if (maincommand=='!')
      {
          // Wifi and MQTT-Broker settings
          // Wifi settings schema [SSID:PASS]
          // Broker settings schema {IP:PORT:USER:PASS}
          // so lets start
          int _start  = _cmd.indexOf("[");
          int _end    = _cmd.indexOf("]");
          if ((_start>=0) && (_end>=0)) 
          {
              // Wifi settings
              String _str_wifi  = _cmd.substring(_start+1,_end);
              int _next         = _str_wifi.indexOf(":");
              String _ssid      = _str_wifi.substring(0,_next);
              String _pass      = _str_wifi.substring(_next+1);
              needReconnect     = true;
              // here some checks could be implemented
               vTaskEnterCritical(&Mutex);
              ssid              = _ssid;
              password          = _pass;
              vTaskExitCritical(&Mutex);

              // Serial.printf("SSID: %s PASS: %s",_ssid.c_str(),_pass.c_str());
              
          }
          _start  = _cmd.indexOf("{");
          _end    = _cmd.indexOf("}");
          if ((_start>=0) && (_end>=0)) 
          {
             // Broker settings
             String _str_broker   = _cmd.substring(_start+1,_end);
             int _next_delim      = _str_broker.indexOf(":",0); // from beginning
             int _mid_delim       = _str_broker.indexOf(":",_next_delim+1);
             int _last_delim      = _str_broker.lastIndexOf(":");
             String _broker_ip    = _str_broker.substring(0,_next_delim);
             String _port         = _str_broker.substring(_next_delim+1,_mid_delim);
             String _user         = _str_broker.substring(_mid_delim+1,_last_delim);
             String _pass         = _str_broker.substring(_last_delim+1);
             
             needReconnect        = true;
             // here some checks could be implemented
             vTaskEnterCritical(&Mutex);
             mqttIP               = _broker_ip;
             mqttUser             = _user;
             mqttPass             = _pass;
             mqttPort             = _port.toInt();
             vTaskExitCritical(&Mutex);
             // Serial.printf("IP: %s PORT: %d USER: %s PASS: %s",_broker_ip.c_str(),mqttPort,_user.c_str(),_pass.c_str());
          }
      }
    }
    SerialCommands.Clear();
  }
  
}
//--------------------------------------------------
void serialReceived( void )
//--------------------------------------------------
{
  static String input ="";
  if (Serial.available()>0)
  {
    byte in = Serial.read();

    if ((in == 0x0A) || (in == 0x0D))// || (in == 0x00))
    {
      //----------------
      // split string 
      //----------------
      String sub;
      while(input.length()>0)
      {
        int next = input.indexOf(",");
        if (next<0) next = input.length();
        sub = input.substring(0,next);
        if (sub.length()>0) 
        {
            SerialCommands.Add(sub);
        }
        input.remove(0,next+1);
      }
      input.clear();
    }
    else input.concat((char)in);
  }
  serialcommands_handler();
}
//-------------------------------------------------------------------
void messageReceived(char* topic, byte* payload, unsigned int length) 
//-------------------------------------------------------------------
{
  // converting topic and payload to String objects
  // maybe its easier parsing it
  // Link to the stringobject documentation
  // https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/
  // static int counter = 0;
  String str_topic(topic);  
  char char_payload[100];
  // copy the every byte to char array
  for (int i = 0; i < length;i++)
    char_payload[i] = (char)payload[i];
  char_payload[length] = 0x00; // ohh, don't forget this ! termination 
  String str_payload(char_payload);
  /*
  if (PSC.getChannelFromTopic(topic)>=0)
  {
      tft.setTextColor(TFT_GREEN,TFT_BLACK); // Set the font colour AND the background colour
      //Serial.println(str_payload);
  }
  else
  {
    tft.setTextColor(TFT_RED,TFT_BLACK); // Set the font colour AND the background colour
  }
  */
  PSC.setChannelData(str_topic,str_payload);
  // Printing out received data on serial port
  //Serial.print("Received [");
  //Serial.print(str_topic);
  //Serial.print("] ");
  //Serial.print(str_payload);

}
//--------------------------------------------
void onTrigger(void)
//--------------------------------------------
{
  Serial.printf("%s",PSC.Excelstream().c_str());
  double t = PSC.getChannelData(1,CHANNEL_MODE_LAST) * 10;
  analog.plotNeedle((int)t,0);
}

//--------------------------------------------
void setup() 
//--------------------------------------------
{
   
    static uint8_t ucParameterToPass;
    struct tm local;
    chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
    sprintf(chipid_str,"%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);
    tft.begin();
    tft.setRotation(1);
    tft.setTextSize(3);
    tft.setCursor(20,50);
    tft.println("RESET");
    
 
    tft.setTextSize(3);
    EEPROM.begin(10);
    Serial.begin(BAUDRATE,SERIAL_8N1);
    #ifndef DEBUG
    Serial.setDebugOutput(false);
    #endif
    // Connect to wifi and Broker
    xTaskCreate( connectTask, "CONNECTOR", 4096, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle);
    //configASSERT( xHandle );
    do{vTaskDelay(10/portTICK_PERIOD_MS);}while(!connected);
    
    configTzTime(TZ_INFO, NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
    getLocalTime(&local, 10000);      // Versuche 10 s lang zu Synchronisieren

    /*
    tft.printf("%02d:%02d:%02d\r",local.tm_hour,local.tm_min,local.tm_sec);
      
    
    WiFi.disconnect();
    for (int i=0; i <= 1000;i++)
    {
      delay(10);
      tft.setCursor(20,50);
      getLocalTime(&local);
      tft.printf("%02d:%02d:%02d\r",local.tm_hour,local.tm_min,local.tm_sec);
      client.loop();
    }
    */
    SerialCommands.Clear();  
    analog.plot();   
}

//--------------------------------------------
void loop() 
//--------------------------------------------
{
  static int state = 0;
  static int once = 0;
  static unsigned long lastMillis  = 0;    
  client.loop(); // must be called periodically to get incomming messages from MQTT-Broker
  serialReceived();
  //delay(10);  // <- fixes some issues with WiFi stability - only if neccessary
  if (!client.connected() || needReconnect) // check connection status continously
  {
    connected = false;
    vTaskResume(xHandle);
    do
    {
        vTaskDelay(1/ portTICK_PERIOD_MS);
        serialReceived();
    }while(!connected);
    delay(10);
    
    //PSC.clearAllChannelData();
    PSC.resubscribeAllChannels();
    needReconnect = false;
    analog.plot();
    //tft.fillScreen(TFT_BLACK);
    //tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Set the font colour AND the background colour
    //tft.setCursor(30,30);
    //tft.setTextSize(2);
    //tft.println("Connection Lost!");
    return;
  }
  if (!once)
  {
    once = 1;
    // testsubscription
    //PSC.setChannelTopic(1,"WROOM/TEMP",CHANNEL_MODE_LAST);
    //PSC.setChannelTopic(2,"WROOM/HUMI",CHANNEL_MODE_LAST);
    PSC.setChannelTopic(1,"Krenn/BK/Volt",CHANNEL_MODE_LAST);
    //PSC.setChannelTopic(2,"WROOM/HUMI",CHANNEL_MODE_LAST);
  }
  
  // Trigger 
  // Timetrigger , AllSignaltrigger, Changetrigger with minval, Leveltrigger
  // Manual Trigger ( Button) , Remote Trigger ( Excel Button) 

  // AllSignaltrigger with Timeout
  /*
  switch(state)
  {
    case 0:
          lastMillis=millis();
          state = 1; 
    case 1:         
          if(PSC.isNewDataOnAllActiveChannels(lastMillis))
            state = 2; 
          if (millis()-lastMillis > 5000 )
            state = 3;
        break;
    case 2:
          //triggered
          onTrigger();          
          state = 0;
        break;
    case 3:
          //timeout
          onTrigger();          
          state = 0;
        break;
  }
  */
  // AllSignaltrigger after Wait
  switch(state)
  {
    case 0:
          lastMillis=millis();
          state = 1; 
    case 1:  
          if (millis()-lastMillis > 2000 )
          {
            lastMillis=millis();
            state = 2;        
          }
        break;
    case 2:
            if(PSC.isNewDataOnAllActiveChannels(lastMillis))
            state = 3; 
            if (millis()-lastMillis > 5000 )
            state = 4;
        break;
    case 3:
          //timeout
          onTrigger();          
          state = 0;
        break;
    case 4:
          //timeout
          onTrigger();          
          state = 0;
        break;
  } 
}

