/*
    Target: ESP32
*/
// -------------------------------------------------------------------------------------------
#include "main.h"

#define MODELNAME             "D-RGB-8X5"
#define MANUFACTORER          "F.Krenn-HTL-ET"
#define DEVICETYPE            "Display-RGB-8x5"
#define VERSION               "1.0.0"

#define FORCE_CONFIG_PORTAL   0
#define DEBUG                 0
#define BAUDRATE              9600
#define TLS_PORT_RANGE_START  8000
#define TLS_PORT_RANGE_END    8999      

// LED Matrix configuration
// 40 Pixels / Pin#13 @ Wemos d1 R32 / 2812Shield
#define led_data_pin arduino_pin13
#define rows_count  8
#define cols_count  5
#define pixel_count rows_count * cols_count

// ------------> Time Server settings <------------------
#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time
// -------------------------------------------------------------------------------------------

Adafruit_NeoPixel leds(pixel_count, led_data_pin, NEO_GRB + NEO_KHZ800); // verified settings
LED_Matrix display(rows_count, cols_count, leds);
MQTTNode node("maqlab", MANUFACTORER, MODELNAME, DEVICETYPE, VERSION);

// definition of eeprom address
#define eeprom_addr_state         0
#define eeprom_addr_mqtt_server   eeprom_addr_state       + 4
#define eeprom_addr_mqtt_port     eeprom_addr_mqtt_server + 32
#define eeprom_addr_mqtt_user     eeprom_addr_mqtt_port   + 15
#define eeprom_addr_mqtt_pass     eeprom_addr_mqtt_user   + 32
#define eeprom_addr_chipid        eeprom_addr_mqtt_pass   + 32
#define eeprom_addr_WiFi_SSID     eeprom_addr_chipid      + 32
#define eeprom_addr_WiFi_pass     eeprom_addr_WiFi_SSID   + 32
#define eeprom_addr_mqtt_root     eeprom_addr_WiFi_pass   + 32
//#define eeprom_addr_mqtt_tls      eeprom_addr_mqtt_root   + 32

#define led_green arduino_pin2
#define led_red   arduino_pin3
#define digital_input arduino_pin12

// -----------------------------------------
//            global variables
// -----------------------------------------
// default values
char szt_mqtt_hostname[32]  = "mqtt-host";
char szt_mqtt_ip[32]        = "0.0.0.0";
char szt_mqtt_port[15]      = "mqtt-port";
char szt_mqtt_user[32]      = "mqtt-username";
char szt_mqtt_pass[32]      = "mqtt-password";
char szt_mqtt_root[32]      = "mqtt-root";

String wifi_ssid            = "";
String wifi_password        = "";
String mqtt_hostname        = ""; 
String mqtt_port            = "";
String mqtt_user            = ""; 
String mqtt_password        = ""; 
String mqtt_root            = "";

// String deviceNameFull;
// String topic_root = mqtt_root + "/";  
boolean needReconnect = false;

// -----------------------------------------
// get a Unique ID ( we use the mac-address )
// -----------------------------------------
uint64_t chipid;
char chipid_str[13];    // 6 Bytes = 12 Chars + \0x00 = 13 Chars

WiFiClient            net_unsec;
WiFiClientSecure      net_secure;
MQTTClient            mqtt(8096); // besser ausrechnen !!
WifiMQTT              mqtt_connection(&mqtt,"LAWIG14","wiesengrund14","techfit.at",8883,"maqlab","maqlab",true);

// prototypes
void messageReceived(String &topic, String &payload);
bool connecting_to_Wifi_and_broker();
void configModeCallback (WiFiManager *myWiFiManager);
void saveConfigCallback (void);

/*
//-------------------------------------------------------------------
bool connecting_to_Wifi_and_broker()
//-------------------------------------------------------------------
{
  int counter = 0;
  Serial.print("WiFi Connecting.");

  while(!WiFi.isConnected())
  {
    digitalWrite(led_green, 1);
    delay(100);
    digitalWrite(led_green, 0);
    Serial.print(".");
    digitalWrite(led_red, 1);
    delay(500);
    digitalWrite(led_red, 0);
    delay(500);
    if (++counter > 10) return(false);
  }
  Serial.println("connected !!");
  // get host ip from host name

  
  Serial.println("Resolve ip adress from hostname: " + mqtt_hostname);
  IPAddress mqtt_host_ip;
  
  counter = 0;
  while (WiFi.hostByName(mqtt_hostname.c_str(), mqtt_host_ip) != 1)
  {
    Serial.print(".");
    digitalWrite(led_red, 1);
    delay(500);
    digitalWrite(led_red, 0);
    delay(500);
    if (++counter > 10) return(false);
  }
  Serial.println("IP-Address: " + mqtt_host_ip.toString()); 
  Serial.print("Connecting to broker with ");
    // we decide on the port range
  int port = mqtt_port.toInt();
  if ((port >= TLS_PORT_RANGE_START) && (port <= TLS_PORT_RANGE_END))
   {
    Serial.print("secure TLS connection.");
    // client.begin(mqtt_host_ip.toString().c_str(), port, net_secure);
    client.begin("172.16.132.34", 8883, net_unsec);
  }
  else
  {
    Serial.print("unsecured connection.");
    client.begin(mqtt_host_ip.toString().c_str(), port, net_unsec);
  }
  

  counter = 0;
  while (!client.connect(chipid_str,mqtt_user.c_str(),mqtt_password.c_str()))
  {
    Serial.print("*");
    digitalWrite(led_red, 1);
    delay(500);
    digitalWrite(led_red, 0);
    delay(500);
    counter++;
    if (++counter > 10) return(false);
  }
  return(true);
}
*/
//-------------------------------------------------------------------
void mqtt_disconnected()
//-------------------------------------------------------------------
{
    Serial.println("*** DISONNECTED ***");
}
//-------------------------------------------------------------------
void mqtt_connected()
//-------------------------------------------------------------------
{
    Serial.println("CONNECTED :-) ");
    mqtt.subscribe("maqlab/#",0);
    Serial.println("Subscribed ");
    //node.subscribe(mqtt);
}
//-------------------------------------------------------------------
void messageReceived(String &topic, String &payload) 
//-------------------------------------------------------------------
{ 
  struct tm local;
  getLocalTime(&local);       // Versuche 10 s lang zu Synchronisieren 
  // Serial.println("RECEIVING");
  // Printing out received message on serial port
  String time = String(local.tm_hour) + ":" + String(local.tm_min) + ":" + String(local.tm_sec);
  Serial.println(time + " Received: " + topic + " " + payload);
  /*
  node.handle_mqtt_message(topic, payload, mqtt);
  if (node.is_message_for_this_device(topic)) 
    display.handle_mqtt_message(topic,payload,mqtt);
  */
}

//--------------------------------------------
void setup() 
//--------------------------------------------
{   
    // delay(2000);
    mqtt_connection.set_onConnected(mqtt_connected);
    mqtt_connection.set_onDisconnected(mqtt_disconnected);  
    Serial.begin(BAUDRATE,SERIAL_8N1);
    Serial.println("\n*** STARTUP after RESET ***");
    chipid=ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
    sprintf(chipid_str,"%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);
    Serial.println("MAX HEAPSIZE: " + String(ESP.getMaxAllocHeap()));
    display.begin();
    bool needConfigPortal = false;
    pinMode(digital_input,INPUT_PULLUP);
    delay(10);
    needConfigPortal = !(bool)digitalRead(digital_input);
    while(!(bool)digitalRead(digital_input))
    {
      Serial.println("Remove Wire");
      delay(200);
    };
   
    Serial.println("Devicename: " + node.get_devicefullname());
    Serial.println("Acessnumber: " + String(node.get_accessnumber()));
    // configure led pins
    pinMode(led_green, OUTPUT);
    pinMode(led_red, OUTPUT);
    digitalWrite(led_red, 0);
    digitalWrite(led_green, 0);
   
    // reading the config from eeprom
    EEPROM.begin(255);
    if (EEPROM.readByte(eeprom_addr_WiFi_SSID) == 0xaa) 
      wifi_ssid        = EEPROM.readString(eeprom_addr_WiFi_SSID+1);
    if (EEPROM.readByte(eeprom_addr_WiFi_pass) == 0xaa) 
      wifi_password    = EEPROM.readString(eeprom_addr_WiFi_pass+1);

    mqtt_hostname    = EEPROM.readString(eeprom_addr_mqtt_server+1);
    mqtt_port        = EEPROM.readString(eeprom_addr_mqtt_port+1);
    mqtt_user        = EEPROM.readString(eeprom_addr_mqtt_user+1);
    mqtt_password    = EEPROM.readString(eeprom_addr_mqtt_pass+1);
    mqtt_root        = EEPROM.readString(eeprom_addr_mqtt_root+1);
   
    if (EEPROM.readByte(eeprom_addr_mqtt_server) == 0xaa) 
      mqtt_hostname.toCharArray(szt_mqtt_hostname,30);
    if (EEPROM.readByte(eeprom_addr_mqtt_port) == 0xaa) 
      mqtt_port.toCharArray(szt_mqtt_port,8);
    if (EEPROM.readByte(eeprom_addr_mqtt_pass) == 0xaa) 
      mqtt_password.toCharArray(szt_mqtt_pass,30);
    if (EEPROM.readByte(eeprom_addr_mqtt_user) == 0xaa) 
      mqtt_user.toCharArray(szt_mqtt_user,30);
    if (EEPROM.readByte(eeprom_addr_mqtt_root) == 0xaa) 
      mqtt_user.toCharArray(szt_mqtt_root,30);
  
    Serial.print("MQTT-HOST: ");
    Serial.println(szt_mqtt_hostname);
    Serial.print("MQTT-PORT: ");
    Serial.println(szt_mqtt_port);
    Serial.print("MQTT-USER: ");
    Serial.println(szt_mqtt_user);
    Serial.print("MQTT-PASSWD: ");
    Serial.println(szt_mqtt_pass);
    Serial.print("MQTT-ROOT: ");
    Serial.println(szt_mqtt_root);

    mqtt_connection.start();
    
    /*
    while (!mqtt_connection.mqtt_is_connected())
    {
      mqtt.loop();
      vTaskDelay(10/portTICK_PERIOD_MS);
    }
    */
    mqtt.onMessage(messageReceived);
   
    //return;
    /*
    do
    {
      
      mqtt.publish("maqlab/ruby/test","ok");
      vTaskDelay(1000/portTICK_PERIOD_MS);
    } while (true);
    */
    // mqtt.publish("maqlab/ruby/test","ok");
    
    /*
    do
    {    
      client.disconnect();
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.enableSTA(true);
      Serial.println("WiFi-SSID: " + wifi_ssid);
      #if (FORCE_CONFIG_PORTAL == 1)
        needConfigPortal = true;
      #endif
      
      if (needConfigPortal)
        Serial.println("*** NEED CONFIG-PORTAL ***");
      else
        Serial.println("*** CONFIG-PORTAL NOT NEEDED ***");

      wifi_ssid  = "HTL-HG";
      wifi_password = "hollabrunn";
      mqtt_hostname = "172.16.132.34";
      mqtt_password = "maqlab";
      mqtt_port = 8883;
      mqtt_root = "maqlab";
      mqtt_user = "maqlab";
      needConfigPortal = false;

      if (needConfigPortal)
      {
        digitalWrite(led_red, 1);
        WiFiManager wifiManager;
        //wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
        
        wifiManager.setConnectTimeout(5);
        wifiManager.setConfigPortalTimeout(500);
        String text = "<p><h2>Accessnumber: " + String(node.get_accessnumber()) + "</h2></p>";
        WiFiManagerParameter custom_text(text.c_str());
        WiFiManagerParameter custom_mqtt_server("server", "mqtt server", szt_mqtt_hostname , 30);
        WiFiManagerParameter custom_mqtt_port("port", "mqtt port", szt_mqtt_port, 13);
        WiFiManagerParameter custom_mqtt_username("user", "mqtt user", szt_mqtt_user , 30);
        WiFiManagerParameter custom_mqtt_password("pass", "mqtt pass", szt_mqtt_pass, 30);
        WiFiManagerParameter custom_mqtt_root("root", "mqtt root", szt_mqtt_root, 30);
        String text_tls = "<div>For secure TLS use ports 8000...8999</div>";
        WiFiManagerParameter custom_text_tls(text_tls.c_str());    
        wifiManager.addParameter(&custom_text);
        wifiManager.addParameter(&custom_mqtt_server);
        wifiManager.addParameter(&custom_mqtt_port);
        wifiManager.addParameter(&custom_text_tls);
        wifiManager.addParameter(&custom_mqtt_username);
        wifiManager.addParameter(&custom_mqtt_password);
        wifiManager.addParameter(&custom_mqtt_root);
        wifiManager.setAPCallback(configModeCallback); 
        wifiManager.setSaveConfigCallback(saveConfigCallback);
        String temp_ssid = "ESP" + String(chipid_str); 
        wifiManager.startConfigPortal(temp_ssid.c_str());
        //read updated parameters
        wifi_ssid      = wifiManager.getSSID();
        wifi_password  = wifiManager.getPassword();
        mqtt_hostname = custom_mqtt_server.getValue();
        mqtt_port     = custom_mqtt_port.getValue();
        mqtt_user     = custom_mqtt_username.getValue();
        mqtt_password = custom_mqtt_password.getValue();
        mqtt_root     = custom_mqtt_root.getValue();
       
        mqtt_hostname.toLowerCase();
            
        digitalWrite(led_red, 0);
        WiFi.enableAP(false);   // remove the AP from the network
      }
      else
      {
        WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str());
      }
    }
    while (!connecting_to_Wifi_and_broker());
   
    digitalWrite(led_green,1);
    Serial.println("connected !!");
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
    EEPROM.writeByte(eeprom_addr_mqtt_root,0xaa);
    EEPROM.writeString(eeprom_addr_mqtt_root+1,mqtt_root);
    EEPROM.commit();

    
    // mqtt.onMessage(messageReceived);
    // node.set_root(mqtt_root);
    // node.set_commandlist("[\"setpixel_rgb\",\"setpixel_hsv\"]");
    // node.subscribe(mqtt); 
    */
}

//--------------------------------------------
void loop() 
//--------------------------------------------
{    
  // static int timer_ms = 0;
  // static int counter = 0;
  
  // mqtt.loop(); // must be called periodically to get incomming messages from MQTT-Broker
  // struct tm local;
  // getLocalTime(&local);       // Versuche 10 s lang zu Synchronisieren 
    
  // vTaskDelay(900/portTICK_PERIOD_MS); // <- fixes some issues with WiFi stability - only if neccessary
  // Serial.println("TIME: " + String(local.tm_sec));
  // detect reconnection 
  /*
  return;
  if (!mqtt.connected() || needReconnect) // check connection status continously
  {
    digitalWrite(led_green,0);
    Serial.println("--> unexpected disconnection <--");
    do
    {
      mqtt.disconnect();
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.enableSTA(true);
      WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str());
    }
    while (!connecting_to_Wifi_and_broker());
    digitalWrite(led_green,1);
    Serial.println("Connected!!");
    node.subscribe(mqtt);
  }
  if ((millis() - timer_ms) > 1000)
  {
    timer_ms = millis();
  }
  */
}
//callbacks
//-------------------------------------------------------------------
void configModeCallback (WiFiManager *myWiFiManager) 
//-------------------------------------------------------------------
{  
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP()); 
  Serial.println(myWiFiManager->getConfigPortalSSID()); 
}
//-------------------------------------------------------------------
void saveConfigCallback () 
//-------------------------------------------------------------------
{
  Serial.println("Should save config");
  Serial.println(WiFi.softAPIP());
}
