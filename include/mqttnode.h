#ifndef _MQTTNODE_H
#define _MQTTNODE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <wifi_mqtt.h>

class MQTTNode
{
   private:
        uint32_t _accessnumber;
        String _manufactorer, _model, _devicetype;
        String _devicefullname, _root;
        String _commandlist;
        DynamicJsonDocument * _doc;
        WifiMQTT* _mqtt;

   public:
        MQTTNode(WifiMQTT* mqtt, const char* root, const char* manufactorer, const char* model, const char* devicetype, const char* version); // constructor
        bool handle_standard_commands(String topic, String payload);    
        bool is_message_for_this_device(String topic);
        void set_root(String root);
        void set_commandlist(String commandlist);
        void subscribe( void);
        int get_accessnumber(void);
        String get_devicefullname(void);
};
#endif