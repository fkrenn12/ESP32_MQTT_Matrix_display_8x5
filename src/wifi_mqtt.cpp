#include <wifi_mqtt.h>

// -----------------------------------------------------------------------------
WifiMQTT::WifiMQTT( MQTTClient* client,
                    String      wifi_ssid, 
                    String      wifi_passwd, 
                    String      mqtt_hostname, 
                    uint32_t    mqtt_port,
                    String      mqtt_user,
                    String      mqtt_passwd,
                    bool        mqtt_use_tls) // constructor 
// -----------------------------------------------------------------------------
{
    // ConnectingTask*
    _client = client;
    task = new ConnectingTask(  client,
                                wifi_ssid, 
                                wifi_passwd, 
                                mqtt_hostname, 
                                mqtt_port,
                                mqtt_user,
                                mqtt_passwd,
                                mqtt_use_tls,
                                &net_unsec,
                                &net_secure);
                                    
    xTaskCreatePinnedToCore(    ConnectingTask::run, 
                                "wifi_mqtt_connect", 
                                8092, 
                                (void*) task, 
                                1, // we need higher task priority than 0
                                &taskHandle,
                                APP_CPU_NUM); 
      
}; 
// -----------------------------------------------------------------------------
bool WifiMQTT::wifi_is_connected(void)
{
    return (task->wifi_is_connected);
}
// -----------------------------------------------------------------------------
bool WifiMQTT::mqtt_is_connected(void)
{
    return (task->mqtt_is_connected);
}
// -----------------------------------------------------------------------------
void WifiMQTT::set_onConnected(void (*funcptr)(void)) 
{
    task->onConnectptr = funcptr;
}
// -----------------------------------------------------------------------------
void WifiMQTT::set_onDisconnected(void (*funcptr)(void)) 
{
    task->onDisconnectptr = funcptr;
}
// -----------------------------------------------------------------------------
void WifiMQTT::start(void) 
{
   vTaskResume(taskHandle);
}
// -----------------------------------------------------------------------------
void WifiMQTT::stop(void) 
{
   vTaskSuspend(taskHandle);
}
// -----------------------------------------------------------------------------
void WifiMQTT::loop(void) 
{
   task->fire_loop = true;
}
// -----------------------------------------------------------------------------
bool WifiMQTT::publish(String& topic, String& payload, bool retain, int qos, unsigned int timeout_ms) 
{
    unsigned int start_millis = millis();
    unsigned int timeout = 50;

    if (timeout_ms > 0)
        timeout = timeout_ms;

    
    while (true)
    {
        if (millis() - start_millis > timeout)
            return (false);

        if (millis() - task->last_loop_millis  > 50)
        {
            _client->publish(topic,payload,retain,qos);
            return (true);
        }
        vTaskSwitchContext();
    }
}
