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
                                1 );   
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