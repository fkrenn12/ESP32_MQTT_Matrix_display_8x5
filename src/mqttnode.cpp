#include <mqttnode.h>

// Member functions definitions including constructor
// ---------------------------------------------------------------------------------------------------------------------
MQTTNode::MQTTNode(const char* root, const char* manufactorer, const char* model, const char* devicetype, const char* version) // constructor
// ---------------------------------------------------------------------------------------------------------------------
{
    uint64_t chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
    _accessnumber = (uint16_t)(chipid>>32);
    _accessnumber %= 1000;
    _accessnumber += 8000;
    _manufactorer = manufactorer;
    _model = model;
    _devicetype = devicetype;
    _devicefullname = String(model) + " V"  + String(version);
    _root = String(root);
}

// ---------------------------------------------------------------------------------------------------------------------
void MQTTNode::handle_mqtt_message(String topic, String payload, MQTTClient& client)
// ---------------------------------------------------------------------------------------------------------------------
{
    if (topic.lastIndexOf("cmd/?") >= 0)
    {
        topic.replace("cmd/?","rep/");
        client.publish(topic + _devicefullname + "/accessnumber",String(_accessnumber));
        return;
    }
    if (is_message_for_this_device(topic))
    {
        
        if (topic.lastIndexOf("/echo") >= 0)
        {
            topic.replace("cmd","rep");
            client.publish(topic,payload);
            return;
        }
        else if (topic.lastIndexOf("/?") >= 0)
        {
            String new_topic;
            String new_payload;
            
            topic.replace("cmd","rep");
            new_topic = topic;
            new_payload = "{'commands':'" + _commandlist +"';'model':'" + _model + "'}";
            client.publish(new_topic, new_payload);
            topic.replace("?","");
            
            // TODO: JSON String for 
            new_topic = topic + "commands";
            client.publish(new_topic,_commandlist);
            new_topic = topic + "manufactorer";
            client.publish(new_topic,_manufactorer);
            new_topic = topic + "model";
            client.publish(new_topic,_model);
            new_topic = topic + "devicetype";
            client.publish(new_topic,_devicetype);
            return;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
bool MQTTNode::is_message_for_this_device(String topic)
// ---------------------------------------------------------------------------------------------------------------------
{
    // matching accessnumber?
    String access_topic_string = "cmd/" + String(_accessnumber) + "/";
    return (topic.lastIndexOf(access_topic_string) >= 0);
}

// ---------------------------------------------------------------------------------------------------------------------
int MQTTNode::get_accessnumber(void)
// ---------------------------------------------------------------------------------------------------------------------
{
    return (_accessnumber);
}

// ---------------------------------------------------------------------------------------------------------------------
String MQTTNode::get_devicefullname(void)
// ---------------------------------------------------------------------------------------------------------------------
{
    return (_devicefullname);
}

// ---------------------------------------------------------------------------------------------------------------------
void MQTTNode::set_root(String root)
// ---------------------------------------------------------------------------------------------------------------------
{
    _root = root;
   _root.trim();
   if (!_root.endsWith("/")) _root = _root + "/";
}

// ---------------------------------------------------------------------------------------------------------------------
void MQTTNode::set_commandlist(String commandlist)
// ---------------------------------------------------------------------------------------------------------------------
{
    commandlist.trim();
    if (commandlist.startsWith("[") &&  commandlist.endsWith("]"))
        _commandlist = commandlist;
}

// ---------------------------------------------------------------------------------------------------------------------
void MQTTNode::subscribe(MQTTClient &client)
// ---------------------------------------------------------------------------------------------------------------------
{
    if (!_root.endsWith("/")) _root = _root + "/";
    String topic_root = _root;
    client.subscribe(topic_root + "+/+/cmd/?",0);
    client.subscribe(topic_root + "+/cmd/?",0);
    client.subscribe(topic_root + "cmd/?",0);
    client.subscribe(topic_root + "+/+/cmd/" + get_accessnumber() + "/#",0);
    client.subscribe(topic_root + "+/cmd/" + get_accessnumber() + "/#",0);
    client.subscribe(topic_root + "cmd/" + get_accessnumber() + "/#",0);
}