#ifndef _WIFI_MQTT_H
#define _WIFI_MQTT_H

#include <Arduino.h>
#include <MQTT.h>
#include <WiFiClientSecure.h>
#include <config.h>

#define TLS_PORT_RANGE_START  8000
#define TLS_PORT_RANGE_END    8999   

#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time
// #define PRINT_DETAILS 
// -----------------------------------------------------------------------------
class ConnectingTask
// -----------------------------------------------------------------------------
{
    private:
        uint32_t            _wifi_status;
        uint32_t            _mqtt_status;
        MQTTClient*         _client;
        IPAddress           _mqtt_host_ip;   
        WiFiClient*         _net_unsec;
        WiFiClientSecure*   _net_secure;
        String              _mqtt_connction_id;
        void                _mqtt_loop(void);
    public: 
        
        String              _wifi_ssid;
        String              _wifi_passwd;
        String              _mqtt_hostname;
        uint32_t            _mqtt_port;
        String              _mqtt_user;
        String              _mqtt_passwd;
        bool                _mqtt_use_tls;
        bool                wifi_is_connected;
        bool                mqtt_is_connected;
        bool                mqtt_is_secure;
        void                (*onConnectptr)(void);
        void                (*onDisconnectptr)(void);

        SemaphoreHandle_t   xMutex;

        
        static void run( void* pvParams )
        {
            ((ConnectingTask*)pvParams)->runInner();
        }
        void runInner()
        {
            // we create this task suspended 
            vTaskSuspend(NULL);
            #ifdef PRINT_DETAILS 
            Serial.println("Task CPU# " + String(xTaskGetAffinity(NULL)));
            #endif
            while ( true )
            {      
                switch (_wifi_status)
                {
                    case 0: // ----------------------------------
                            // Make sure that we are disconnected
                            // and start wifi (again)            
                            // ----------------------------------              
                            wifi_is_connected = false;
                            WiFi.disconnect();
                            WiFi.mode(WIFI_OFF);
                            vTaskDelay(50/portTICK_PERIOD_MS);
                            WiFi.mode(WIFI_STA);
                            WiFi.enableSTA(true); 
                            #ifdef PRINT_DETAILS  
                            Serial.println("WIFI:" + String(_wifi_ssid) + " " + String(_wifi_passwd));
                            #endif
                            WiFi.begin(_wifi_ssid.c_str(),_wifi_passwd.c_str());                                   
                            vTaskDelay(500/portTICK_PERIOD_MS);
                            _wifi_status = 1;
                            break;

                    case 1: // -------------------
                            // Wait for connection
                            // -------------------                      
                            if (WiFi.status() == WL_CONNECTED)        
                            {   
                                _wifi_status = 2;
                                // synchronize local time with time from time server
                                struct tm local;
                                configTzTime(TZ_INFO, NTP_SERVER); // syncronize ESP32 with NTP
                                getLocalTime(&local, 10000);       // try 10s                                
                            }
                            else vTaskDelay(100/portTICK_PERIOD_MS);                         
                            break;
                            
                    case 2: // -----------------------------
                            // Check connection periodically
                            // -----------------------------
                            wifi_is_connected = true;
                            if (WiFi.status() != WL_CONNECTED)
                            {   
                                _wifi_status = 0;
                                wifi_is_connected = false;
                                vTaskDelay(500/portTICK_PERIOD_MS);
                            }           
                            break;
                    default: break;
                    vTaskSwitchContext();                 
                }

                switch (_mqtt_status)
                {
                    case 0: // ----------------------------------
                            // Initial - Wait for WiFi connection
                            // ----------------------------------
                            _client->disconnect();
                            _mqtt_loop();
                            mqtt_is_connected = false;
                            // wait for Wifi connection established
                            if (wifi_is_connected) _mqtt_status = 1;
                            break;
                    case 1: // ------------------------
                            // Resolving hostname to IP
                            // ------------------------
                            if (!wifi_is_connected) {_mqtt_status = 0;break;}
                            _mqtt_loop();
                            if (WiFi.hostByName(_mqtt_hostname.c_str(), _mqtt_host_ip) == 1)
                                _mqtt_status = 2;
                            else vTaskDelay(200/portTICK_PERIOD_MS);
                            break;

                    case 2: // --------------------
                            // Connecting to broker
                            // --------------------
                            #ifdef PRINT_DETAILS 
                            Serial.println("MQTT-credentials: " + String(_mqtt_hostname) + ":" + String(_mqtt_port));
                            Serial.println("MQTT-credentials: " + String(_mqtt_user + " " + String(_mqtt_passwd + " " + String(_mqtt_use_tls))));
                            Serial.println("MQTT_ID:" + _mqtt_connction_id);
                            #endif
                            if (!wifi_is_connected) {_mqtt_status = 0;break;}
                            if (_mqtt_use_tls)
                                _client->begin(_mqtt_host_ip.toString().c_str(), (int)_mqtt_port, *_net_secure);
                            else
                                _client->begin(_mqtt_host_ip.toString().c_str(), (int)_mqtt_port, *_net_unsec);
                            _mqtt_status = 3;
                            //_mqtt_loop();
                            vTaskDelay(500/portTICK_PERIOD_MS);
                            break;

                    case 3: // -----------------------------
                            // Wait for connection to broker
                            // -----------------------------
                            vTaskDelay(1000/portTICK_PERIOD_MS);
                            if (!wifi_is_connected) {_mqtt_status = 0;break;}
                            _mqtt_loop();
                            if (_client->connect(_mqtt_connction_id.c_str(),_mqtt_user.c_str(),_mqtt_passwd.c_str()))
                            {
                                vTaskDelay(10/portTICK_PERIOD_MS);
                                _mqtt_status = 4;
                                mqtt_is_connected = true;
                                if (onConnectptr != NULL)
                                    (*onConnectptr)();
                            }   
                            // if connection could not be established 
                            // we must start from the beginning, otherwise 
                            // will we see a system crash
                            else _mqtt_status = 0;                       
                            break;

                    case 4: // --------------------------------------
                            // Check connection to broker continously
                            // -------------------------------------- 
                                    
                            if (xSemaphoreTake(xMutex,100/portTICK_PERIOD_MS))
                            {
                                if (!_client->connected() || !wifi_is_connected)
                                {
                                    _mqtt_status = 0;
                                    mqtt_is_connected = false;
                                    if (onDisconnectptr != NULL)
                                        (*onDisconnectptr)();
                                }
                                else
                                {
                                    _client->loop(); 
                                }
                                vTaskDelay(10/portTICK_PERIOD_MS);
                                xSemaphoreGive(xMutex);
                            }
                            break;
                    default: break;
                }
                vTaskDelay(10/portTICK_PERIOD_MS);
            }
        }
        // constructor
        ConnectingTask(MQTTClient*  client,
                       WiFiClient*       net_unsec,
                       WiFiClientSecure* net_secure)
        {
            _wifi_status    = 0;
            _mqtt_status    = 0;
            _client         = client;
            _net_secure     = net_secure;
            _net_unsec      = net_unsec;
            
            _mqtt_connction_id = "ESP" + String((uint16_t)(random(0,1000))) + String((uint16_t)(ESP.getEfuseMac()>>32));
            onConnectptr    = NULL;
            onDisconnectptr = NULL;
            xMutex = xSemaphoreCreateMutex();
        }        
};

// -------------------------------------------------------------
class WifiMQTT
{
   private:
    TaskHandle_t            taskHandle;
    ConnectingTask*         task;
    WiFiClient              net_unsec;
    WiFiClientSecure        net_secure;
    
    
   public:
    WifiMQTT(); // constructor
    MQTTClient*  client;
    bool wifi_is_connected();
    bool mqtt_is_connected(); 
    bool mqtt_is_secure();   
    void start();
    void stop();
    void onConnected(void (*funcptr)(void));  
    void onDisconnected(void (*funcptr)(void)); 
    void onMessage(void (*funcptr)(String&,String&));
    void config(Config& config);  
    bool publish(String& topic, String& payload, bool retain, int qos, unsigned int timeout_ms);
    bool publish(const char* topic, const char*  payload, bool retain, int qos, unsigned int timeout_ms);
    bool subscribe(String& topic, int qos, unsigned int timeout_ms );
    bool subscribe(const char* topic, int qos, unsigned int timeout_ms );
};
// -------------------------------------------------------------
#endif