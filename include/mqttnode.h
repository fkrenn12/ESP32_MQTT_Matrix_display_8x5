#ifndef _MQTTNODE_H
#define _MQTTNODE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <MQTT.h>

class MQTTNode
{
   private:
        int _accessnumber;
        String _manufactorer, _model, _devicetype;
        String _devicefullname, _root;
        String _commandlist;
        DynamicJsonDocument * _doc;
   public:
        MQTTNode(const char* root, const char* manufactorer, const char* model, const char* devicetype, const char* version); // constructor
        void handle_mqtt_message(String topic, String payload, MQTTClient &client);     
        bool is_message_for_this_device(String topic);
        void set_root(String root);
        void set_commandlist(String commandlist);
        void subscribe( MQTTClient &client);
        int get_accessnumber(void);
        String get_devicefullname(void);
};
#endif