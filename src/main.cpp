#define versionNumber "1.0.0"
#define deviceName "ESP32 R4"
#define configPreamble "Qa9dBMx"

//#include <WiFi.h>      //ESP32 Core WiFi Library 
#include <WiFiClientSecure.h>
#include <WebServer.h> 
#include <DNSServer.h> 
#include <WiFiManager.h>   // WiFi Configuration Magic (  https://github.com/zhouhan0126/DNSServer---esp32  ) >>  https://github.com/zhouhan0126/DNSServer---esp32  (ORIGINAL)
#include <PubSubClient.h> // MQTT Library
#include <MQTT.h>
#include "SPI.h"
#include "connecting.h"
#include <EEPROM.h>
#include "driver/uart.h"
/*
String xval = getValue(myString, ':', 0);
String yval = getValue(myString, ':', 1);

Serial.println("Y:" + yval);
Serial.print("X:" + xval);
Convert String to int

int xvalue = xvalue.toInt(xval);
int yvalue = yvalue.toInt(yval);
*/
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



// char blynk_token[34] = "";
// The extra parameters to be configured (can be either global or just in the setup)
// After connecting, parameter.getValue() will get you the configured value
// id/name placeholder/prompt default length

// WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 34);
//#define DEBUG 1
//--------------------------------------------
#define BAUDRATE 9600

//--------------------------------------------
// Time Server setting
//--------------------------------------------
#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time
//--------------------------------------------
// ------------> Wifi settings <------------------
String  ssid            = "LAWIG14";        //Enter SSID
String  password        = "wiesengrund14";  //Enter Password

//const char  ssid[]      = "LAWIG14"; //Enter SSID
//const char  password[]  = "wiesengrund14"; //Enter Password

// --------> MQTT-Broker settings <---------------

String deviceNameFull;  
char mqtt_server[20];
char mqtt_port[6] = "1883";
char mqtt_user[20] = "user";
char mqtt_pass[20] = "password";

const int relay1      = 14;   // IO14 Pin for Relay1 using the Relay Shield by Seed Studio
const int relay2      = 27;   // IO27 Pin for Relay2 using the Relay Shield by Seed Studio
const int relay3      = 16;   // IO16 Pin for Relay3 using the Relay Shield by Seed Studio
const int relay4      = 17;   // IO17 Pin for Relay4 using the Relay Shield by Seed Studio

String mqtt_hostname    = "techfit.at"; 
String mqttIP           = ""; 
String mqttUser         = "maqlab"; // franz
String mqttPass         = "maqlab"; // franz

int mqttPort     = 1883; //8883
int accessnumber = 0000;
// -----------------------------------------
// global
// -----------------------------------------
boolean needReconnect = false;

// -----------------------------------------
// get a Unique ID ( we use the mac-address )
// -----------------------------------------
uint64_t chipid;
char chipid_str[13];    // 6 Bytes = 12 Chars + \0x00 = 13 Chars
//char topic[256];      // char buffer for topic used several times in code

// WiFiClient            net;
WiFiClientSecure      netsec;
MQTTClient            client;
// prototypes
void messageReceived(String &topic, String &payload) ;
void configModeCallback (WiFiManager *myWiFiManager);
void saveConfigCallback (void);

//-------------------------------------------------------------------
void messageReceived(String &topic, String &payload) 
//-------------------------------------------------------------------
{
  // converting topic and payload to String objects
  // maybe its easier parsing it
  // Link to the stringobject documentation
  // https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/
  // static int counter = 0;

  // Printing out received data on serial port
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(payload);
  bool handled = false;
  if (topic.lastIndexOf("cmd/?") >= 0)
  {
    topic.replace("cmd/?","");
    Serial.println(topic);
    client.publish(topic + "rep/" + deviceNameFull + "/accessnumber","0000");
    return;
  }
  // matching accessnumber?
  String access;
  access = "cmd/0000/";  // hier fehlt noch das Umwandeln des int Wertes

  if (topic.lastIndexOf(access) < 0)
  {
    Serial.println("Access number not matched!!");
    // this is a command message for an other device
    // nothing to do !!
    return;
  }
  else if (topic.lastIndexOf("rel/1") >= 0)
  {
    payload.toLowerCase();
    if ( payload.lastIndexOf("on") >= 0 || payload.lastIndexOf("1") >= 0) 
    {
      Serial.println("Relais 1 ON");
      digitalWrite(relay1,1);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 1 OFF");
      digitalWrite(relay1,0);
      handled = true;
    }
  }
  else if (topic.lastIndexOf("rel/2") >= 0)
  {
    payload.toLowerCase();
    if ( payload.lastIndexOf("on") >= 0 || payload.lastIndexOf("1") >= 0) 
    {
      Serial.println("Relais 2 ON");
      digitalWrite(relay2,1);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 2 OFF");
      digitalWrite(relay2,0);
      handled = true;
    }
  }
  else if (topic.lastIndexOf("rel/3") >= 0)
  {
    payload.toLowerCase();
    if ( payload.lastIndexOf("on") >= 0 || payload.lastIndexOf("1") >= 0) 
    {
      Serial.println("Relais 3 ON");
      digitalWrite(relay3,1);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 3 OFF");
      digitalWrite(relay3,0);
      handled = true;
    }
  }
  else if (topic.lastIndexOf("rel/4") >= 0)
  {
    payload.toLowerCase();
    if ( payload.lastIndexOf("on") >= 0 || payload.lastIndexOf("1") >= 0) 
    {
      Serial.println("Relais 4 ON");
      digitalWrite(relay4,1);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 4 OFF");
      digitalWrite(relay4,0);
      handled = true;
    }
  }
 
  topic.replace("cmd","rep");
  if (handled)
  {
      Serial.println("HANDLED");
      Serial.println(topic);
      client.publish(topic,"ACCEPTED");
  }
  else
  {
      Serial.println("NOT HANDLED");
      client.publish(topic,"ERROR");
  }
}
//--------------------------------------------
void setup() 
//--------------------------------------------
{   
    deviceNameFull = deviceName;
    deviceNameFull.concat(" V");
    deviceNameFull.concat(versionNumber);

    chipid=ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
    sprintf(chipid_str,"%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);
   
    // configre relay pins
    pinMode(relay1, OUTPUT); 
    pinMode(relay2, OUTPUT);
    pinMode(relay3, OUTPUT);
    pinMode(relay4, OUTPUT);
    digitalWrite(relay1,0);
    digitalWrite(relay2,0);
    digitalWrite(relay3,0);
    digitalWrite(relay4,0);

    Serial.begin(BAUDRATE,SERIAL_8N1);
    Serial.println("\n*** STARTUP after RESET ***");
    Serial.println((unsigned int)chipid % 1000);
    Serial.println(chipid_str);
    Serial.println(deviceNameFull);
    String mqttConfigStr;
    // TODO: reading the config from eeprom
    EEPROM.begin(255);
    mqttConfigStr = EEPROM.readString(0);
    // check if valid string
    // int ix = mqttConfigStr.indexOf(configPreamble);
    WiFi.mode(WIFI_AP_STA);
    
    //EEPROM.writeString
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(10);
    wifiManager.setConnectTimeout(1);
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server , 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    // wifiManager.addParameter(&custom_blynk_token);
    // wifiManager.resetSettings();
    // ESP.restart();
    // wifiManager.startConfigPortal("HALLO CONFIG", "12345678");
    wifiManager.setAPCallback(configModeCallback); 
    wifiManager.setSaveConfigCallback(saveConfigCallback); 

    // wifiManager.autoConnect("MQTT 4 RELAY Config", "12345678"); 
    wifiManager.autoConnect(); 
    Serial.println(wifiManager.getSSID()); //imprime o SSID criado da rede</p><p>}</p><p>//callback que indica que salvamos uma nova rede para se conectar (modo estação)
    Serial.println(wifiManager.getPassword());
    ssid      = wifiManager.getSSID();
    password  = wifiManager.getPassword();
    
   
    //read updated parameters
    //strcpy(mqtt_server, custom_mqtt_server.getValue());
    //strcpy(mqtt_port, custom_mqtt_port.getValue());
    String mqtt_hostname = custom_mqtt_server.getValue();
    
    Serial.print("MQTT_SERVER:");
    Serial.println(mqtt_server);
    Serial.print("MQTT_PORT:");
    Serial.println(mqtt_port);
    
    if (!WiFi.isConnected())
    {
      delay(2000);
      ESP.restart();
    }
    WiFi.enableAP(false);   // remove the AP from the network
    Serial.println("WiFi Connected !!");
    // get host ip from host name
    // WiFi.hostByName(mhostname, mserver);
    Serial.println("Connecting to Broker");
    client.begin("94.16.117.246" , 8883, netsec);
    client.onMessage(messageReceived);
    while (!client.connect(chipid_str,"franz","franz"))
    {
      Serial.print("*");
      delay(1000);
    }
    Serial.println("Connected!!");
    // client.subscribe("/labor");
    client.subscribe("maqlab/+/+/cmd/#");
    client.subscribe("maqlab/+/cmd/#");
    
    struct tm local;
    configTzTime(TZ_INFO, NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
    getLocalTime(&local, 10000);      // Versuche 10 s lang zu Synchronisieren
    
}

//--------------------------------------------
void loop() 
//--------------------------------------------
{    
  // static int counter = 0;
  client.loop(); // must be called periodically to get incomming messages from MQTT-Broker
  
  delay(10);  // <- fixes some issues with WiFi stability - only if neccessary
  if (!client.connected() || needReconnect) // check connection status continously
  {
    Serial.println("--> unexpected disconnection <--");
    client.disconnect();
    Serial.print("WiFi Connecting");
    while(!WiFi.isConnected())
    {
      Serial.print(".");
      delay(1000);
    }
    Serial.println("WiFi connected");
    Serial.println("Reconnecting to Broker");
    client.begin("94.16.117.246" , 8883, netsec);
    client.onMessage(messageReceived);
    while (!client.connect(chipid_str,"franz","franz"))
    {
      Serial.print("*");
      delay(1000);
    }
    Serial.println("Connected!!");
    // client.subscribe("/labor");
    client.subscribe("maqlab/+/+/cmd/#");
    client.subscribe("maqlab/+/cmd/#");
  }
}
//callback que indica que o ESP entrou no modo AP
void configModeCallback (WiFiManager *myWiFiManager) 
{  
//  Serial.println("Entered config mode");
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); //imprime o SSID criado da rede</p><p>}</p><p>//callback que indica que salvamos uma nova rede para se conectar (modo estação)
  Serial.println(myWiFiManager->getPassword());
}
void saveConfigCallback () 
{
//  Serial.println("Should save config");
  Serial.println("Configuração salva");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
}
