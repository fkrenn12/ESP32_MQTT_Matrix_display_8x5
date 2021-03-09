#include <filesystem.h>

Config::Config(void)
{
  // default config values
  wifi_ssid       = "SSID";
  wifi_pass       = "password"; 
  mqtt_hostname   = "yourbroker.com";
  mqtt_port       = 1883;
  mqtt_password   = "pass";
  mqtt_user       = "user";
  mqtt_root       = "user";
  auto_tls        = true;
  tls             = false;
}
// -----------------------------------------
bool InitalizeFileSystem(bool print_status)
// -----------------------------------------
{
  bool init_ok = false;
  
  for (int i = 1; i < 5; i++)
  {
    init_ok = SPIFFS.begin();
    if (init_ok) break;
    else 
    {
      if (print_status)  
      {
        Serial.println("Formatting SPIFFS file system...attempt #" +String(i));
      }
      SPIFFS.format();  
    }
  }
  init_ok = SPIFFS.begin();
  if (print_status)
  {
    if (init_ok) 
    { Serial.println("SPIFFS ready"); } 
    else 
    { Serial.println("SPIFFS not ready"); }
  }
  // only for testing
  SPIFFS.remove (CONFIG_FILENAME) ;
  return init_ok;
}

// --------------------------------------------
bool existConfigFile(void)
// --------------------------------------------
{
  return (SPIFFS.exists (CONFIG_FILENAME));
}

// --------------------------------------------
bool createDefaultConfigFile(bool print_status)
// --------------------------------------------
{
  File config;
  // check, if config file already exists
  if (!(SPIFFS.exists (CONFIG_FILENAME) ))  
  { 
    // it does not exist - so we create a new config file  
    config = SPIFFS.open(CONFIG_FILENAME, "w"); 
    if (!config)
    {
        if (print_status)
        {
          Serial.println("Could not create file: " + String(CONFIG_FILENAME));
        }
        return (false);
    }
    Config default_config;
    storeConfig(default_config);
  }
  return (SPIFFS.exists (CONFIG_FILENAME));
}

// --------------------------------------------
bool loadConfig(Config& config)
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
  config.wifi_pass        = doc["wifi"]["password"].as<String>();        
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
bool storeConfig(Config& config)
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
  jsondoc["wifi"]["ssid"] = config.wifi_ssid;
  jsondoc["wifi"]["pass"] = config.wifi_pass;
  jsondoc["mqtt"]["hostname"] = config.mqtt_hostname;
  jsondoc["mqtt"]["port"] = config.mqtt_port;
  jsondoc["mqtt"]["auto_tls"] = config.auto_tls;
  jsondoc["mqtt"]["tls"] = config.tls;
  jsondoc["mqtt"]["user"] = config.mqtt_user;
  jsondoc["mqtt"]["pass"] = config.mqtt_password;
  jsondoc["mqtt"]["root"] = config.mqtt_root;
  String jsonstring;
  serializeJson(jsondoc, jsonstring);
  f.println(jsonstring);
  f.close();
  return(true);
}
