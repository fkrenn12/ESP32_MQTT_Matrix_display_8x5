#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#define     CONFIG_FILENAME "/config.json"
#include    <Arduino.h>
#include    <SPIFFS.h>
#include    <ArduinoJson.h>

class Config
{
    public: 
        String      wifi_ssid;
        String      wifi_pass;
        String      mqtt_hostname;
        uint32_t    mqtt_port;          
        String      mqtt_user;           
        String      mqtt_password;      
        String      mqtt_root; 
        bool        auto_tls;
        bool        tls;          
        Config(void);
};

bool InitalizeFileSystem(bool);
bool createDefaultConfigFile(bool print_status);
String getConfig(bool print_status);
bool existConfigFile(void);
bool storeConfig(Config& config);
#endif