/*
    Target: ESP32
*/
// -------------------------------------------------------------------------------------------
#define DEBUG           0
#define versionNumber   "1.0.0"
#define deviceName      "ESP32 R4"
#define BAUDRATE        9600
// ------------> Time Server settings <------------------
#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time
// -------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WebServer.h> 
#include <DNSServer.h> 
#include <WiFiManager.h>    
// WiFi Configuration Magic (  https://github.com/zhouhan0126/DNSServer---esp32  ) 
// https://github.com/zhouhan0126/DNSServer---esp32  (ORIGINAL)
#include <MQTT.h>
#include <EEPROM.h>
// definitions of digital pins
#define pin2 26
#define pin3 25
#define pin8 12
#define pin9 13
// definitions of relay port pins ( relay shield )
#define relay1 14
#define relay2 27
#define relay3 16
#define relay4 17
// definition of eeprom address
#define eeprom_addr_relay_state   0
#define eeprom_addr_mqtt_server   eeprom_addr_relay_state + 4
#define eeprom_addr_mqtt_port     eeprom_addr_mqtt_server + 32
#define eeprom_addr_mqtt_user     eeprom_addr_mqtt_port   + 10
#define eeprom_addr_mqtt_pass     eeprom_addr_mqtt_user   + 32
#define eeprom_addr_chipid        eeprom_addr_mqtt_pass   + 32
#define eeprom_addr_WiFi_SSID     eeprom_addr_chipid      + 32
#define eeprom_addr_WiFi_pass     eeprom_addr_WiFi_SSID   + 32

#define led_green pin2
#define led_red   pin3

// -----------------------------------------
//            global variables
// -----------------------------------------
// default values
char szt_mqtt_hostname[32]  = "mqtt-broker.com";
char szt_mqtt_ip[32]        = "0.0.0.0";
char szt_mqtt_port[10]      = "8883";
char szt_mqtt_user[32]      = "username";
char szt_mqtt_pass[32]      = "password";

String wifi_ssid        = "";
String wifi_password    = "";
String mqtt_hostname    = "techfit.at"; 
String mqtt_port        = "";
String mqtt_user        = "maqlab"; 
String mqtt_password    = "maqlab"; 

String deviceNameFull;  

int accessnumber = 0000;

boolean needReconnect = false;

// -----------------------------------------
// get a Unique ID ( we use the mac-address )
// -----------------------------------------
uint64_t chipid;
char chipid_str[13];    // 6 Bytes = 12 Chars + \0x00 = 13 Chars

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
    client.publish(topic + "rep/" + deviceNameFull + "/accessnumber",String(accessnumber));
    return;
  }
  // matching accessnumber?
  String access;
  access = "cmd/";
  access.concat(String(accessnumber)); // hier fehlt noch das Umwandeln des int Wertes
  access.concat("/");  

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
      EEPROM.writeByte(0,0xa1);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 1 OFF");
      digitalWrite(relay1,0);
      EEPROM.writeByte(0,0xa0);
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
      EEPROM.writeByte(1,0xa3);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 2 OFF");
      digitalWrite(relay2,0);
      EEPROM.writeByte(1,0xa2);
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
      EEPROM.writeByte(2,0xa5);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 3 OFF");
      digitalWrite(relay3,0);
      EEPROM.writeByte(2,0xa4);
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
      EEPROM.writeByte(3,0xa9);
      handled = true;
    }
    if ( payload.lastIndexOf("off") >= 0 || payload.lastIndexOf("0") >= 0)   
    {
      Serial.println("Relais 4 OFF");
      digitalWrite(relay4,0);
      EEPROM.writeByte(3,0xa8);
      handled = true;
    }
  }
 
  topic.replace("cmd","rep");
  if (handled)
  {
      Serial.println("HANDLED");
      Serial.println(topic);
      client.publish(topic,"ACCEPTED");
      EEPROM.commit();
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
    accessnumber = (int)(chipid % 1000) + 8000;
    sprintf(chipid_str,"%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);
    // configure led pins
    pinMode(led_green, OUTPUT);
    pinMode(led_red, OUTPUT);
    digitalWrite(led_red, 0);
    digitalWrite(led_green, 0);
    // configre relay pins
    pinMode(relay1, OUTPUT); 
    pinMode(relay2, OUTPUT);
    pinMode(relay3, OUTPUT);
    pinMode(relay4, OUTPUT);
    delay(100);
    digitalWrite(relay1,0);
    digitalWrite(relay2,0);
    digitalWrite(relay3,0);
    digitalWrite(relay4,0);

    Serial.begin(BAUDRATE,SERIAL_8N1);
    Serial.println("\n*** STARTUP after RESET ***");
    Serial.println((unsigned int)chipid % 1000);
    Serial.println(deviceNameFull);
    // TODO: reading the config from eeprom
    EEPROM.begin(255);
    String wifi_ssid      = "";
    String wifi_password  = "";
    if (EEPROM.readByte(eeprom_addr_WiFi_SSID) == 0xaa) 
      wifi_ssid        = EEPROM.readString(eeprom_addr_WiFi_SSID+1);
    if (EEPROM.readByte(eeprom_addr_WiFi_pass) == 0xaa) 
      wifi_password    = EEPROM.readString(eeprom_addr_WiFi_pass+1);

    String mqtt_hostname    = EEPROM.readString(eeprom_addr_mqtt_server+1);
    String mqtt_ip          = ""; 
    String mqtt_port        = EEPROM.readString(eeprom_addr_mqtt_port+1);
    String mqtt_user        = EEPROM.readString(eeprom_addr_mqtt_user+1);
    String mqtt_password    = EEPROM.readString(eeprom_addr_mqtt_pass+1);

    if (EEPROM.readByte(eeprom_addr_mqtt_server) == 0xaa) 
      mqtt_hostname.toCharArray(szt_mqtt_hostname,30);
    if (EEPROM.readByte(eeprom_addr_mqtt_port) == 0xaa) 
      mqtt_port.toCharArray(szt_mqtt_port,8);
    if (EEPROM.readByte(eeprom_addr_mqtt_pass) == 0xaa) 
      mqtt_password.toCharArray(szt_mqtt_pass,30);
    if (EEPROM.readByte(eeprom_addr_mqtt_user) == 0xaa) 
      mqtt_user.toCharArray(szt_mqtt_user,30);
    
    Serial.print("MQTT-HOSTNAME:");
    Serial.println(szt_mqtt_hostname);
    Serial.print("MQTT-PORT:");
    Serial.println(szt_mqtt_port);
    Serial.print("MQTT-USER:");
    Serial.println(szt_mqtt_user);
    Serial.print("MQTT-PASSWD:");
    Serial.println(szt_mqtt_pass);

    // reading the saved relay states and switch the relays to the saved state
    for (int i=0; i < 4; i++)
    {
      char eebyte;
      eebyte = EEPROM.readByte(i);
      if ((eebyte & 0xF0)  != 0xa0) continue;  // not a valid byte
      eebyte = eebyte & 0x0F;   // clear the validation nibble
      switch(eebyte)
      {
        case 0: digitalWrite(relay1,0); break;
        case 1: digitalWrite(relay1,1); break;
        case 2: digitalWrite(relay2,0); break;
        case 3: digitalWrite(relay2,1); break;
        case 4: digitalWrite(relay3,0); break;
        case 5: digitalWrite(relay3,1); break;
        case 8: digitalWrite(relay4,0); break;
        case 9: digitalWrite(relay4,1); break;
        default: break;
      }
    }
    WiFi.mode(WIFI_STA);
    Serial.println(wifi_ssid.c_str());
    bool needConfigPortal = (wifi_ssid=="");
    if (!needConfigPortal)
    {
      // try to connect to Wifi
      WiFi.begin( wifi_ssid.c_str(), wifi_password.c_str());
      int counter = 0;
      Serial.print("Connect to Wifi");
      while(!WiFi.isConnected())
      {
        delay(1000);
        Serial.print(".");

        if (++counter > 10) break;
      }
      Serial.println("Connected!!");
      IPAddress mqtt_host_ip;
      needConfigPortal = (WiFi.hostByName(mqtt_hostname.c_str(), mqtt_host_ip) != 1);
      needConfigPortal |= (!WiFi.isConnected());
    }
    if (needConfigPortal)
      Serial.println("NEED CONFIGPORTAL");
    else
      Serial.println("NOT NEEDED CONFIGPORTAL");

    // EEPROM.writeString(eeprom_addr_WiFi_SSID,"hallo");
    // EEPROM.commit();
    
    // EEPROM.writeByte(0,0xa1);
    // EEPROM.commit();
    if (needConfigPortal)
    {
      digitalWrite(led_red, 1);
      WiFiManager wifiManager;
      wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
      wifiManager.setConnectTimeout(5);
      wifiManager.setConfigPortalTimeout(200);
      WiFiManagerParameter custom_mqtt_server("server", "mqtt server", szt_mqtt_hostname , 30);
      WiFiManagerParameter custom_mqtt_port("port", "mqtt port", szt_mqtt_port, 8);
      WiFiManagerParameter custom_mqtt_username("user", "mqtt user", szt_mqtt_user , 30);
      WiFiManagerParameter custom_mqtt_password("pass", "mqtt pass", szt_mqtt_pass, 30);
      wifiManager.addParameter(&custom_mqtt_server);
      wifiManager.addParameter(&custom_mqtt_port);
      wifiManager.addParameter(&custom_mqtt_username);
      wifiManager.addParameter(&custom_mqtt_password);
      // wifiManager.resetSettings();
      // ESP.restart();
      wifiManager.setAPCallback(configModeCallback); 
      wifiManager.setSaveConfigCallback(saveConfigCallback); 
      wifiManager.startConfigPortal();
      // wifiManager.autoConnect("MQTT 4 RELAY Config", "12345678"); 
      // wifiManager.autoConnect(); 
      
      Serial.println(wifiManager.getSSID()); //imprime o SSID criado da rede</p><p>}</p><p>//callback que indica que salvamos uma nova rede para se conectar (modo estação)
      Serial.println(wifiManager.getPassword());
      wifi_ssid      = wifiManager.getSSID();
      wifi_password  = wifiManager.getPassword();
      //read updated parameters
      //strcpy(mqtt_server, custom_mqtt_server.getValue());
      //strcpy(mqtt_port, custom_mqtt_port.getValue());
      mqtt_hostname = custom_mqtt_server.getValue();
      mqtt_port     = custom_mqtt_port.getValue();
      mqtt_user     = custom_mqtt_username.getValue();
      mqtt_password = custom_mqtt_password.getValue();

      digitalWrite(led_red, 0);
    }
    WiFi.enableAP(false);   // remove the AP from the network
    if (!WiFi.isConnected())
    {
      ESP.restart();
    }
    Serial.println("WiFi Connected !!");
    // get host ip from host name
    IPAddress mqtt_host_ip;
    if (WiFi.hostByName(mqtt_hostname.c_str(), mqtt_host_ip) != 1)
      ESP.restart();

    Serial.println(mqtt_host_ip.toString());

    Serial.println("Connecting to Broker");
    client.begin(mqtt_host_ip.toString().c_str(), mqtt_port.toInt(), netsec);
    client.onMessage(messageReceived);
    while (!client.connect(chipid_str,mqtt_user.c_str(),mqtt_password.c_str()))
    {
      Serial.print("*");
      delay(1000);
    }
    digitalWrite(led_green,1);
    Serial.println("Connected!!");
    // save wifi and broker credentials to EEPROM
    EEPROM.writeByte(eeprom_addr_WiFi_SSID,0xaa);
    EEPROM.writeString(eeprom_addr_WiFi_SSID+1,wifi_ssid);
    EEPROM.writeByte(eeprom_addr_WiFi_pass,0xaa);
    EEPROM.writeString(eeprom_addr_WiFi_pass+1,wifi_password);
    EEPROM.writeByte(eeprom_addr_mqtt_pass,0xaa);
    EEPROM.writeString(eeprom_addr_mqtt_pass+1,mqtt_password);
    EEPROM.writeByte(eeprom_addr_mqtt_user,0xaa);
    EEPROM.writeString(eeprom_addr_mqtt_user+1,mqtt_user);
    EEPROM.writeByte(eeprom_addr_mqtt_server,0xaa);
    EEPROM.writeString(eeprom_addr_mqtt_server+1,mqtt_hostname);
    EEPROM.writeByte(eeprom_addr_mqtt_port,0xaa);
    EEPROM.writeString(eeprom_addr_mqtt_port+1,mqtt_port);
    EEPROM.commit();

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
  static int timer_ms = 0;
  client.loop(); // must be called periodically to get incomming messages from MQTT-Broker
  delay(10);  // <- fixes some issues with WiFi stability - only if neccessary
  if (!client.connected() || needReconnect) // check connection status continously
  {
    digitalWrite(led_green,0);
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
    digitalWrite(led_green,1);
    Serial.println("Connected!!");
    // client.subscribe("/labor");
    client.subscribe("maqlab/+/+/cmd/#");
    client.subscribe("maqlab/+/cmd/#");
  }
  if ((millis() - timer_ms) > 1000)
  {
    timer_ms = millis();
    if (digitalRead(relay1))
      client.publish("maqlab/1/1/rep","{'status':{'rel1':'1'}}");
    else
      client.publish("maqlab/1/1/rep","{'status':{'rel1':'0'}}");
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
