/*
    Target: ESP32
*/
// -------------------------------------------------------------------------------------------
#define FORCE_CONFIG_PORTAL   0
#define DEBUG                 0
#define versionNumber         "1.0.0"
#define MODELNAME             "D-RGB-8X5"
#define MANUFACTORER          "F.Krenn-HTL-ET"
#define DEVICETYPE            "Display-RGB-8x5"
#define BAUDRATE              9600

// ------------> Time Server settings <------------------
#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time
// -------------------------------------------------------------------------------------------
#include <Arduino.h>

#include <WiFiClientSecure.h>
#include <WiFiManager.h>    
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
// WiFi Configuration Magic (  https://github.com/zhouhan0126/DNSServer---esp32  ) 
// https://github.com/zhouhan0126/DNSServer---esp32  (ORIGINAL)
#include <MQTT.h>
#include <EEPROM.h>
// 40 Pixels / Pin#18 @ ESP32-wroom / 2812Shield
Adafruit_NeoPixel matrix(40, 18, NEO_GRB + NEO_KHZ800); // verified settings
DynamicJsonDocument doc(1024);

#define number_of_pixels_in_row   8
#define number_of_rows            5
#define number_of_pixels_in_col   5
#define number_of_cols            8
#define number_of_pixels          number_of_pixels_in_row * number_of_pixels_in_col

// definitions of digital pins
#define pin2 26
#define pin3 25
#define pin8 12
#define pin9 13
#define pin13 18

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
#define eeprom_addr_mqtt_tls      eeprom_addr_mqtt_root   + 32

#define led_green pin2
#define led_red   pin3
#define digital_input pin13

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
char szt_mqtt_tls[10]       = "mqtt_tls";

String wifi_ssid            = "";
String wifi_password        = "";
String mqtt_hostname        = ""; 
String mqtt_port            = "";
String mqtt_user            = ""; 
String mqtt_password        = ""; 
String mqtt_root            = "";
String mqtt_tls             = "";

String deviceNameFull;
String topic_root = mqtt_root + "/";  

unsigned int accessnumber = 0;

boolean needReconnect = false;

// -----------------------------------------
// get a Unique ID ( we use the mac-address )
// -----------------------------------------
uint64_t chipid;
char chipid_str[13];    // 6 Bytes = 12 Chars + \0x00 = 13 Chars

WiFiClient            net_unsec;
WiFiClientSecure      net_secure;
MQTTClient            client;
// prototypes
void messageReceived(String &topic, String &payload);
//bool connecting_to_Wifi_and_broker();
void configModeCallback (WiFiManager *myWiFiManager);
void saveConfigCallback (void);

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
  Serial.println("TLS:" + mqtt_tls);
  Serial.print("Connecting to broker with ");
  mqtt_tls.toLowerCase();
  if (mqtt_tls == "tls")
  {
    Serial.print("secure TLS connection.");
    client.begin(mqtt_host_ip.toString().c_str(), mqtt_port.toInt(), net_secure);
  }
  else
  {
    Serial.print("unsecured connection.");
    client.begin(mqtt_host_ip.toString().c_str(), mqtt_port.toInt(), net_unsec);
  }
  client.onMessage(messageReceived);

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
  access.concat(String(accessnumber)); 
  access.concat("/");  

  if (topic.lastIndexOf(access) < 0)
  {
    Serial.println("Access number not matched!!");
    // this is a command message for an other device
    // nothing to do !!
    return;
  }

  topic.replace("cmd","rep");
  if (topic.lastIndexOf("/echo") >= 0)
  {
      client.publish(topic,payload);
      return;
  }
  else if (topic.lastIndexOf("/?") >= 0)
  {
      topic.replace("?","");
      String new_topic;
      new_topic = topic + "commands";
      client.publish(new_topic,"{[]}");
      new_topic = topic + "manufactorer";
      client.publish(new_topic,MANUFACTORER);
      new_topic = topic + "model";
      client.publish(new_topic,MODELNAME);
      new_topic = topic + "devicetype";
      client.publish(new_topic,DEVICETYPE);
      return;
  }
  else if (topic.lastIndexOf("/setpixel_rgb/") >= 0)
  {
      Serial.println("PAYLOAD: "+ payload);
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
          matrix.setPixelColor(pixel, color); // Set pixel 'c' to value 'color'
          matrix.show();
        }      
      }
  }
  else if (topic.lastIndexOf("/setpixel_hsv/") >= 0)
  {
       Serial.println("PAYLOAD: "+ payload);
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
          Serial.println("HUE " + String(hue));
          Serial.println("SAT " + String(sat));
          Serial.println("VAL " + String(val));
       }
       uint32_t rgbcolor = matrix.gamma32(matrix.ColorHSV(hue, sat, val));
       Serial.println("RGB: " + String(rgbcolor));
       //int color = payload.toInt();
       int index = topic.lastIndexOf("/");
       // setting the color auf of a single pixel
       // extract the pixel number from the topic
       // and validate it 
       int pixel = topic.substring( index + 1 ).toInt();
       if (pixel >= 0 && pixel < number_of_pixels) 
       {
          matrix.setPixelColor(pixel, rgbcolor); // Set pixel 'c' to value 'color'
          matrix.show();
       }      
  }
  else if (topic.lastIndexOf("/json") >= 0)
  {
       deserializeJson(doc, payload);      
  }
  String _topic    = topic_root + "rep/" + String(accessnumber); 
}

//--------------------------------------------
void setup() 
//--------------------------------------------
{   
    bool needConfigPortal = false;
    pinMode(digital_input,INPUT_PULLUP);
    delay(10);
    needConfigPortal = !(bool)digitalRead(digital_input);
    while(!(bool)digitalRead(digital_input));

    matrix.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    matrix.show();            // Turn OFF all pixels ASAP
    matrix.setBrightness(30); // Set BRIGHTNESS to about 1/5 (max = 255)
    //matrix.setPixelColor(0, 20000); // Set pixel 'c' to value 'color'
    //matrix.show();
    matrix.setPixelColor(0, 0, 0, 255);
    matrix.show();
    delay(1000);
    for (int i = 0; i < 40; i++)
      matrix.setPixelColor(i, matrix.Color(80, 80, 80)); // G B R
    matrix.show();
    delay(1000);
    for (int i = 0; i < 40; i++)
      matrix.setPixelColor(i, 0); // Set pixel 'c' to value 'color'
    matrix.show();
    
    
    deviceNameFull = MODELNAME;
    deviceNameFull.concat(" V");
    deviceNameFull.concat(versionNumber);
    chipid=ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).

    accessnumber = ((unsigned int)chipid % 1000) + 8000;
    accessnumber = (uint16_t)(chipid>>32);
    accessnumber %= 1000;
    accessnumber += 8000;
    sprintf(chipid_str,"%04X%08X",(uint16_t)(chipid>>32),(uint32_t)chipid);

    Serial.begin(BAUDRATE,SERIAL_8N1);
    Serial.println("\n*** STARTUP after RESET ***");
    Serial.println("Devicename: " + deviceNameFull);
    Serial.println("ChipId: " + String(chipid_str));
    Serial.println("Acessnumber: " + String(accessnumber));
    // configure led pins
    pinMode(led_green, OUTPUT);
    pinMode(led_red, OUTPUT);
    digitalWrite(led_red, 0);
    digitalWrite(led_green, 0);
   
    // reading the config from eeprom
    EEPROM.begin(255);
    wifi_ssid = "";
    wifi_password  = "";

    if (EEPROM.readByte(eeprom_addr_WiFi_SSID) == 0xaa) 
      wifi_ssid        = EEPROM.readString(eeprom_addr_WiFi_SSID+1);
    if (EEPROM.readByte(eeprom_addr_WiFi_pass) == 0xaa) 
      wifi_password    = EEPROM.readString(eeprom_addr_WiFi_pass+1);

    mqtt_hostname    = EEPROM.readString(eeprom_addr_mqtt_server+1);
    mqtt_port        = EEPROM.readString(eeprom_addr_mqtt_port+1);
    mqtt_user        = EEPROM.readString(eeprom_addr_mqtt_user+1);
    mqtt_password    = EEPROM.readString(eeprom_addr_mqtt_pass+1);
    mqtt_root        = EEPROM.readString(eeprom_addr_mqtt_root+1);
    mqtt_tls         = EEPROM.readString(eeprom_addr_mqtt_tls+1);

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
    if (EEPROM.readByte(eeprom_addr_mqtt_tls) == 0xaa) 
      mqtt_user.toCharArray(szt_mqtt_root,10);
    
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
    Serial.print("MQTT-TLS: ");
    Serial.println(szt_mqtt_tls);

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

      if (needConfigPortal)
      {
        digitalWrite(led_red, 1);
        WiFiManager wifiManager;
        //wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
        
        wifiManager.setConnectTimeout(5);
        wifiManager.setConfigPortalTimeout(500);
        String text = "<p><h2>Accessnumber: " + String(accessnumber) + "</h2></p>";
        WiFiManagerParameter custom_text(text.c_str());
        WiFiManagerParameter custom_mqtt_server("server", "mqtt server", szt_mqtt_hostname , 30);
        WiFiManagerParameter custom_mqtt_port("port", "mqtt port", szt_mqtt_port, 13);
        WiFiManagerParameter custom_mqtt_username("user", "mqtt user", szt_mqtt_user , 30);
        WiFiManagerParameter custom_mqtt_password("pass", "mqtt pass", szt_mqtt_pass, 30);
        WiFiManagerParameter custom_mqtt_root("root", "mqtt root", szt_mqtt_root, 30);
        String text_tls = "<p>Write tls into the following field if you want TLS support or let it blank if not</p>";
        WiFiManagerParameter custom_text_tls(text_tls.c_str());
        WiFiManagerParameter custom_mqtt_tls("tls", "mqtt tls", szt_mqtt_tls,10);
        wifiManager.addParameter(&custom_text_tls);
        wifiManager.addParameter(&custom_mqtt_tls);
        wifiManager.addParameter(&custom_text);
        wifiManager.addParameter(&custom_mqtt_server);
        wifiManager.addParameter(&custom_mqtt_port);
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
        mqtt_tls      = custom_mqtt_tls.getValue();

        mqtt_hostname.toLowerCase();
        mqtt_tls.toLowerCase();
        ;
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
    EEPROM.writeByte(eeprom_addr_mqtt_tls,0xaa);
    EEPROM.writeString(eeprom_addr_mqtt_tls+1,mqtt_tls);
    EEPROM.commit();


    topic_root = mqtt_root + "/";
    client.subscribe(topic_root + "+/+/cmd/#");
    Serial.println("Subscribed to: " + topic_root + "+/+/cmd/#");
    client.subscribe(topic_root + "+/cmd/#");
    Serial.println("Subscribed to: " + topic_root + "+/cmd/#");
    client.subscribe(topic_root + "cmd/#");
    Serial.println("Subscribed to: " + topic_root + "cmd/#");
     
    struct tm local;
    configTzTime(TZ_INFO, NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
    getLocalTime(&local, 10000);       // Versuche 10 s lang zu Synchronisieren   
}

//--------------------------------------------
void loop() 
//--------------------------------------------
{    
  static int timer_ms = 0;
  // static int counter = 0;
  client.loop(); // must be called periodically to get incomming messages from MQTT-Broker
  delay(10);  // <- fixes some issues with WiFi stability - only if neccessary
  if (!client.connected() || needReconnect) // check connection status continously
  {
    digitalWrite(led_green,0);
    Serial.println("--> unexpected disconnection <--");
    do
    {
      client.disconnect();
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.enableSTA(true);
      WiFi.begin(wifi_ssid.c_str(),wifi_password.c_str());
    }
    while (!connecting_to_Wifi_and_broker());
    digitalWrite(led_green,1);
    Serial.println("Connected!!");

    client.subscribe(topic_root + "+/+/cmd/#");
    client.subscribe(topic_root + "+/cmd/#");
    client.subscribe(topic_root + "cmd/#");
  }
  if ((millis() - timer_ms) > 1000)
  {
    timer_ms = millis();
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
