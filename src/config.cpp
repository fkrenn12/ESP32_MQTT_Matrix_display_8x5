/*
  Author: Franz Krenn
*/
#include <config.h>


// ----------------------------------------
Config::Config(void)
// ----------------------------------------
{
  uint64_t u64_chipid;
  char chipid_str[13];
  // default config values
  wifi_ssid       = "SSID";
  wifi_pass       = "password"; 
  mqtt_hostname   = "mqtt.broker.com";
  mqtt_port       = 1883;
  mqtt_user       = "user";
  mqtt_password   = "pass";
  mqtt_root       = "root";
  auto_tls        = true;  // using port range to detect using TLS or not 
                           // see #define TLS_PORT_RANGE_START and TLS_PORT_RANGE_STOP
                           // in wifi_mqtt.h
  tls             = false; // only used if auto_tls = false 
  u64_chipid=ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  sprintf(chipid_str,"%04X%08X",(uint16_t)(u64_chipid>>32),(uint32_t)u64_chipid);
  chipid = String(chipid_str);
}

// -----------------------------------------
bool Config::initialize(void)
// -----------------------------------------
{
  bool init_ok = false;
  
  for (int i = 1; i < 5; i++)
  {
    init_ok = SPIFFS.begin();
    if (init_ok) break;
    else { SPIFFS.format(); }
  }
  init_ok = SPIFFS.begin();
  // only for testing
  // SPIFFS.remove (CONFIG_FILENAME) ;
  return init_ok;
}

// --------------------------------------------
bool Config::exist(void)
// --------------------------------------------
{
  return (SPIFFS.exists (CONFIG_FILENAME));
}

// --------------------------------------------
bool Config::create(void)
// --------------------------------------------
{
  File config;
  // check, if config file already exists
  if (!(SPIFFS.exists (CONFIG_FILENAME) ))  
  { 
    // it does not exist - so we create a new config file  
    config = SPIFFS.open(CONFIG_FILENAME, "w"); 
    if (!config) {return (false);}
    Config default_config;
    store(default_config);
  }
  return (SPIFFS.exists (CONFIG_FILENAME));
}

// --------------------------------------------
bool Config::load(Config& config)
// --------------------------------------------
{
  String jsonstring; 
  File f;
  // check, if config file already exists
  if (SPIFFS.exists (CONFIG_FILENAME))
  { 
    f = SPIFFS.open(CONFIG_FILENAME, "r");
    jsonstring = f.readString();
  }
  else return (false);

  DynamicJsonDocument doc(8096);
  deserializeJson(doc, jsonstring);
  config.wifi_ssid        = doc["wifi"]["ssid"].as<String>();
  config.wifi_pass        = doc["wifi"]["pass"].as<String>();        
  config.mqtt_hostname    = doc["mqtt"]["hostname"].as<String>();
  config.mqtt_port        = doc["mqtt"]["port"].as<uint32_t>();
  config.mqtt_user        = doc["mqtt"]["user"].as<String>();
  config.mqtt_password    = doc["mqtt"]["pass"].as<String>();
  config.mqtt_root        = doc["mqtt"]["root"].as<String>();
  config.auto_tls         = doc["mqtt"]["auto_tls"].as<bool>();
  config.tls              = doc["mqtt"]["tls"].as<bool>();
  return(true);
}

// --------------------------------------------
bool Config::store(Config& config)
// --------------------------------------------
{
  File f;
  // check, if config file already exists
  if (SPIFFS.exists (CONFIG_FILENAME))
  { 
    f = SPIFFS.open(CONFIG_FILENAME, "w");
  }
  else return (false);
  DynamicJsonDocument jsondoc(8096);
  jsondoc["wifi"]["ssid"]     = config.wifi_ssid;
  jsondoc["wifi"]["pass"]     = config.wifi_pass;
  jsondoc["mqtt"]["hostname"] = config.mqtt_hostname;
  jsondoc["mqtt"]["port"]     = config.mqtt_port;
  jsondoc["mqtt"]["auto_tls"] = config.auto_tls;
  jsondoc["mqtt"]["tls"]      = config.tls;
  jsondoc["mqtt"]["user"]     = config.mqtt_user;
  jsondoc["mqtt"]["pass"]     = config.mqtt_password;
  jsondoc["mqtt"]["root"]     = config.mqtt_root;
  String jsonstring;
  serializeJson(jsondoc, jsonstring);
  f.println(jsonstring);
  f.close();
  return(true);
}

// -------------------------------------------------------
bool Config::portal(Config& config, uint32_t accessnumber)
// -------------------------------------------------------
{
  WiFiManager wifiManager;
  //wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
  config.mqtt_hostname.toCharArray(szt_mqtt_hostname,30);
  String(config.mqtt_port).toCharArray(szt_mqtt_port,8);
  config.mqtt_password.toCharArray(szt_mqtt_pass,30);
  config.mqtt_user.toCharArray(szt_mqtt_user,30);
  config.mqtt_user.toCharArray(szt_mqtt_root,30);

  wifiManager.setConnectTimeout(5);
  wifiManager.setConfigPortalTimeout(500);
  String text = "<p><h2>Accessnumber: " + String(accessnumber) + "</h2></p>";
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
  String temp_ssid = "ESP" + chipid; 
  wifiManager.startConfigPortal(temp_ssid.c_str());
  
  //read updated parameters
  config.wifi_ssid     = wifiManager.getSSID();
  config.wifi_pass     = wifiManager.getPassword();
  config.mqtt_hostname = custom_mqtt_server.getValue();
  config.mqtt_port     = (uint32_t)String(custom_mqtt_port.getValue()).toInt();
  config.mqtt_user     = custom_mqtt_username.getValue();
  config.mqtt_password = custom_mqtt_password.getValue();
  config.mqtt_root     = custom_mqtt_root.getValue();
  
  config.mqtt_hostname.toLowerCase();
  WiFi.enableAP(false);   // remove the AP from the network
  return(true);
}

// -------------------------------------------------------
void Config::printout(Config& config)
// -------------------------------------------------------
{
  Serial.println("Configuration - Printout");
  Serial.println("WIFI-SSID " + config.wifi_ssid);
  Serial.println("WIFI-PASS " + config.wifi_pass);
  Serial.println("MQTT-HOSTNAME " + config.mqtt_hostname);
  Serial.println("MQTT-PASSWORD " + config.mqtt_password);
}
