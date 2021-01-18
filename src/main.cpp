//#include <WiFi.h>      //ESP32 Core WiFi Library 
#include <WiFiClientSecure.h>
#include <WebServer.h> 
#include <DNSServer.h> 
#include <WiFiManager.h>   // WiFi Configuration Magic (  https://github.com/zhouhan0126/DNSServer---esp32  ) >>  https://github.com/zhouhan0126/DNSServer---esp32  (ORIGINAL)
#include <PubSubClient.h> // MQTT Library
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

#define configPreamble "Qa9dBMx"
char mqtt_server[20];
char mqtt_port[6] = "1883";
char mqtt_user[20] = "user";
char mqtt_pass[20] = "password";
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


String mqttIP    = "94.16.117.246"; 
String mqttUser  = "labor";
String mqttPass  = "labor"; 

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

WiFiClient            net;
PubSubClient          client(net);
TaskHandle_t          xHandle = NULL;
// prototypes
void messageReceived(char*, byte*, unsigned int);
void configModeCallback (WiFiManager *myWiFiManager);
void saveConfigCallback (void);

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
  // Printing out received data on serial port
  Serial.print("Received [");
  Serial.print(str_topic);
  Serial.print("] ");
  Serial.print(str_payload);

}
//--------------------------------------------
void setup() 
//--------------------------------------------
{ 
    String mqttConfigStr;
    // reading the config from eeprom
    EEPROM.begin(255);
    mqttConfigStr = EEPROM.readString(0);
    // check if valid string
    int ix = mqttConfigStr.indexOf(configPreamble);
    
    //EEPROM.writeString
    WiFiManager wifiManager;
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    //wifiManager.addParameter(&custom_blynk_token);

    
    // wifiManager.resetSettings();
    // ESP.restart();
    Serial.begin(BAUDRATE,SERIAL_8N1);
    //wifiManager.startConfigPortal("HALLO CONFIG", "12345678");
    wifiManager.setAPCallback(configModeCallback); 
    wifiManager.setSaveConfigCallback(saveConfigCallback); 
    wifiManager.autoConnect("HALLO CONFIG", "12345678"); 
    Serial.println(wifiManager.getSSID()); //imprime o SSID criado da rede</p><p>}</p><p>//callback que indica que salvamos uma nova rede para se conectar (modo estação)
    Serial.println(wifiManager.getPassword());
    ssid      = wifiManager.getSSID();
    password  = wifiManager.getPassword();
    //read updated parameters
    //strcpy(mqtt_server, custom_mqtt_server.getValue());
    //strcpy(mqtt_port, custom_mqtt_port.getValue());
    Serial.print("MQTT_SERVER:");
    Serial.println(mqtt_server);
    Serial.print("MQTT_PORT:");
    Serial.println(mqtt_port);

    if (WiFi.isConnected())
    {
      Serial.println("OHHH CONNECTED");
      WiFi.disconnect();
    }
    WiFi.disconnect();
    
    
    //wifiManager.autoConnect("ESP_AP", "12345678"); 

    static uint8_t ucParameterToPass;
    struct tm local;
    chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
    sprintf(chipid_str,"%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);
  
    


    #ifndef DEBUG
    Serial.setDebugOutput(false);
    #endif
    Serial.print("Testing serial");
    Serial.println(chipid_str);

    // Connect to wifi and Broker
    xTaskCreate( connectTask, "CONNECTOR", 4096, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle);
    //configASSERT( xHandle );
    do{vTaskDelay(10/portTICK_PERIOD_MS);}while(!connected);
    Serial.print("Connected");
    client.publish("labor/test","Hallo");
    configTzTime(TZ_INFO, NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
    getLocalTime(&local, 10000);      // Versuche 10 s lang zu Synchronisieren
    
}

//--------------------------------------------
void loop() 
//--------------------------------------------
{    
  static int counter = 0;
  
  /*
  client.loop(); // must be called periodically to get incomming messages from MQTT-Broker
  // delay(10);  // <- fixes some issues with WiFi stability - only if neccessary
  if (!client.connected() || needReconnect) // check connection status continously
  {
    connected = false;
    Serial.print("Client disconnected");
    vTaskResume(xHandle);
    do
    {
        vTaskDelay(1/ portTICK_PERIOD_MS);
    }while(!connected);
    Serial.print("Reconnected");
    delay(10);
    
    needReconnect = false;
    return;
  }
  */
  Serial.println(counter++);
  delay(100);
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
