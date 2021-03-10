#ifndef _CONFIG_H
#define _CONFIG_H

#define     CONFIG_FILENAME "/config.json"
#include    <Arduino.h>
#include    <SPIFFS.h>
#include    <ArduinoJson.h>
#include    <WiFiManager.h>  

class Config
{
    private:
        char szt_mqtt_hostname[32]  = "mqtt-host";
        char szt_mqtt_ip[32]        = "0.0.0.0";
        char szt_mqtt_port[15]      = "mqtt-port";
        char szt_mqtt_user[32]      = "mqtt-username";
        char szt_mqtt_pass[32]      = "mqtt-password";
        char szt_mqtt_root[32]      = "mqtt-root";
    public: 
        String      chipid;
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
        bool initialize(void);
        bool create(void);
        bool exist(void);
        bool store(Config& config);
        bool load(Config& config);
        bool portal(Config& config, uint32_t accessnumber);
        void printout(Config& config);
};
#endif