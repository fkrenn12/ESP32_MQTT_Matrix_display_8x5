#include <WiFi.h>
#include <PubSubClient.h> // MQTT Library
#include "SPI.h"
#include "TFT_eSPI.h"
#include "pubsubcontroller.h"
#include "listlib.h"
#include "analogmeter.h"
#include <EEPROM.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
// #include <iostream>
//--------------------------------------------
#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"
#define BAUDRATE 250000
// ------------> Wifi settings <------------------
String  ssid            = "LAWIG14";        //Enter SSID
String  password        = "wiesengrund14";  //Enter Password
/*
const char  ssid[]      = "LAWIG14"; //Enter SSID
const char  password[]  = "wiesengrund14"; //Enter Password
*/
// --------> MQTT-Broker settings <---------------
/*
const char mqttIP[]     = "94.16.117.246"; 
const char mqttUser[]   = "labor";
const char mqttPass[]   = "labor"; 
*/
String mqttIP           = "91.132.147.143"; 
String mqttUser         = "franz";
String mqttPass         = "FK_s10rr6fr"; 
int mqttPort            = 1883;
// -----------------------------------------
// global
// -----------------------------------------
boolean needReconnect = false;
portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;

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

// prototypes
int connect(); // Connecting to Wifi and MQTT-Broker
void messageReceived(char*, byte*, unsigned int);
void serialcommands_handler(void);
void serialreceive_handler( void * pvParameters );

//--------------------------------------------------
int connect() // Connecting to Wifi and MQTT-Broker
//--------------------------------------------------
{
  int state   = 0;
  int timer   = 0;
  // Wifi
  // Wait some time to connect to wifi
  WiFi.begin(ssid.c_str(), password.c_str());
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_BLACK, TFT_RED); // Set the font colour AND the background colour
  tft.setTextSize(2);
  tft.setCursor(0, 0); // Set cursor at top left of screen
  tft.print("Wifi connect to:\nSSID: ");
  tft.println(ssid);
  tft.println("PASS: ----------");
  do
  {
    taskYIELD();
    vTaskDelay(1/ portTICK_PERIOD_MS);
    serialcommands_handler();
    switch (state)
    {
      case 0://
              if (WiFi.status() == WL_CONNECTED)  
              state = 1; 
            timer++;
            if (timer % 1000 == 0) tft.print(".");
            if (timer > 10000) 
            {
              tft.println("\nNot connnected!");
              tft.println("Check Wifi\ncredential!");
              vTaskDelay(2000/ portTICK_PERIOD_MS);
              return 0;
            }            
            break;
      case 1://
                client.setServer(mqttIP.c_str(), mqttPort);
                client.setCallback(messageReceived);
                tft.println("MQTT Broker-IP:");
                tft.println(mqttIP);
                tft.print("Connect");
                timer = 0;
                state = 2;
            break;
      case 2://
              client.setServer(mqttIP.c_str(), mqttPort);
              if (client.connect(chipid_str, mqttUser.c_str(), mqttPass.c_str())) // this function blocks a long time
                state = 3;          
            timer++;
            if (timer > 5) 
            {
              tft.println("\nNot connnected!");
              tft.println("Check Broker\nsettings!");
              vTaskDelay(2000/ portTICK_PERIOD_MS);
              return 0;
            } 
            break;
      case 3://
            tft.println("\nConnected!");
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Set the font colour AND the background colour
            return 1;
      default:
            break;
    }
  }while(true);

  // we make an topic subscription for data input
  //sprintf(topic,"%s%s",mqttPreample,"/#"); // # means ALL topics
  //sprintf(topic,"%s%s","","#"); // # means ALL topics
  //client.subscribe("#");
  //delay(2000);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Set the font colour AND the background colour
  // client.unsubscribe(topic);  // this would be an unsubcription of a topic
  return 1;
}
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
      //Serial.printf("#%s",_cmd.c_str());
      //if (i==len-1) Serial.print("\n\r");
      //else Serial.print(",");

      // first we look at the command selector
      // its the first character
      char maincommand = _cmd.charAt(0);
      //Serial.printf("+%s",_cmd.c_str());
      _cmd.remove(0,1);
      if (maincommand=='#')
      {
        // delete the first character
        //_cmd.remove(0,1);
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
        //Serial.printf("Remaining: %s\n",_cmd.c_str());
        //Serial.printf("Channel:%i Topic: %s\n",_ichannel,_topic.c_str());
        // now, its time to store channel and topic into the list
        PSC.setChannelTopic(_ichannel,_topic);
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
          // Wifi settings betwenn [SSID:PASS]
          // Broker settings betwenn {IP:USER:PASS}
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
              ssid              = _ssid;
              password          = _pass;

              //Serial.printf("SSID: %s PASS: %s",_ssid.c_str(),_pass.c_str());
              
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
             mqttIP               = _broker_ip;
             mqttUser             = _user;
             mqttPass             = _pass;
             mqttPort             = _port.toInt();
             //Serial.printf("IP: %s PORT: %d USER: %s PASS: %s",_broker_ip.c_str(),mqttPort,_user.c_str(),_pass.c_str());
          }
      }
    }
    SerialCommands.Clear();
  }
  
}
//--------------------------------------------------
void serialreceive_handler( void * pvParameters )
//--------------------------------------------------
{
  static String input ="";
  // empty input buffer 
  //if (Serial.available()>0) 
  //    Serial.read();

  //for(;;)
  {
    //taskYIELD();
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
            //sub[sub.length()]=0;           
            //portENTER_CRITICAL(&myMutex);
            //if (sub.startsWith("#") || sub.startsWith("!") || sub.startsWith("$") || sub.startsWith("*") || sub.startsWith("%")) 
              SerialCommands.Add(sub);
               //Serial.printf("#%s",sub.c_str());
            //portEXIT_CRITICAL(&myMutex);
          }
          input.remove(0,next+1);
        }
        input.clear();
      }
      else input.concat((char)in);
    }
    
  }
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

  /*
  tft.setCursor(0, 0); // Set cursor at top left of screen
  tft.printf("%d",++counter);
  int y = PSC.getChannelFromTopic(topic) * 30;
  if (y>=0)
  {
    tft.setCursor(0, y); // Set cursor at top left of screen
    tft.print("[");
    tft.print(str_topic);
    tft.print("] ");
    tft.print(str_payload);
    tft.println();
  }
  */
}
void setup() 
{
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;
    chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
    sprintf(chipid_str,"%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);
    tft.begin();
    tft.setRotation(1);
    tft.setTextSize(3);
    tft.setCursor(20,50);
    tft.print("RESET");
    delay(1000);
    tft.setTextSize(3);
    EEPROM.begin(10);
    Serial.begin(BAUDRATE,SERIAL_8N1);
    // Connect to wifi and Broker
    while (connect()==0);
    //PSC.setChannel(1,"Krenn/RELAY/1",1);
    SerialCommands.Clear();
    PSC.setChannelTopic(1,"WROOM/TEMP");
    PSC.setChannelTopic(2,"WROOM/HUMI");
    analog.plot();
    //xTaskCreatePinnedToCore( serialreceive_handler, "HANDLER", 1024, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle,0 );
    //configASSERT( xHandle );
    
}

void loop() 
{
    static uint8_t ucParameterToPass;
    static unsigned long lastMillis  = 0;    
    client.loop(); // must be called periodically to get incomming messages from MQTT-Broker
    serialreceive_handler(&ucParameterToPass);
    serialcommands_handler();
    //delay(10);  // <- fixes some issues with WiFi stability - only if neccessary
    if (!client.connected() || needReconnect) // check connection status continously
    {
      WiFi.disconnect();
      while (connect()==0);
      PSC.clearChannelData();
      PSC.resubscribeChannels();
      needReconnect = false;
      analog.plot();
      return;
    }
    
    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (millis() - lastMillis > 1000) // 1sec = 1000ms 
    {
      lastMillis = millis();
      /*
      for ( int i = 0; i < CHANNEL_MAX;i++)
      {
        Serial.printf("%d: %s-%i-%s\n\r",i+1,PSC.topic[i].c_str(),PSC.state[i],PSC.data[i].c_str());
      }
      */
     Serial.printf("%s",PSC.Excelstream().c_str());
     double t = (PSC.getChannelData(1,CHANNEL_MODE_LAST)).toDouble();
     analog.plotNeedle((int)t,0);
     
     //Serial.println(PSC.data[1]);
     //PSC.Excelstream();
      //... doing code every 1000ms
    }    
}

