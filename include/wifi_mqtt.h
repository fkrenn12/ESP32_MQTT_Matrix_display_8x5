#ifndef _WIFI_MQTT_H
#define _WIFI_MQTT_H

#include <Arduino.h>
#include <MQTT.h>
#include <WiFiClientSecure.h>

#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time

// -----------------------------------------------------------------------------
class ConnectingTask
// -----------------------------------------------------------------------------
{
    private:
        uint32_t            _wifi_status;
        uint32_t            _mqtt_status;
        String              _wifi_ssid;
        String              _wifi_passwd;
        String              _mqtt_hostname;
        uint32_t            _mqtt_port;
        String              _mqtt_user;
        String              _mqtt_passwd;
        bool                _mqtt_use_tls;
        IPAddress           _mqtt_host_ip;
        MQTTClient*         _client;
        WiFiClient*         _net_unsec;
        WiFiClientSecure*   _net_secure;
        String              _mqtt_connction_id;
       
       

    public: 
        bool        wifi_is_connected;
        bool        mqtt_is_connected;
        bool        mqtt_is_secure;
        void        (*onConnectptr)(void);
        void        (*onDisconnectptr)(void);

        static void run( void* pvParams )
        {
            ((ConnectingTask*)pvParams)->runInner();
        }
        void runInner()
        {
            // we wait 3 seconds to let the setup() initialize
            // all hardware drivers e.g. Serial
            // it could also be done smarter with suspend/resume
            //vTaskDelay(3000/portTICK_PERIOD_MS);
            vTaskSuspend(NULL);
            while ( true )
            {      
                // vTaskDelay(10/portTICK_PERIOD_MS); 
                // Serial.print("A");
                // _client->loop();
                // Serial.println("B");
                // Serial.println("Wifi-Task #" + String(_wifi_status));
                // Serial.println("MQTT-Task #" + String(_mqtt_status));
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
                            Serial.println("WIFI:" + String(_wifi_ssid) + " " + String(_wifi_passwd));
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

                if (!wifi_is_connected) _mqtt_status = 0;

                switch (_mqtt_status)
                {
                    case 0: // ----------------------------------
                            // Initial - Wait for WiFi connection
                            // ----------------------------------
                            _client->disconnect();
                            _client->loop();
                            mqtt_is_connected = false;
                            // wait for Wifi connection established
                            if (wifi_is_connected) _mqtt_status = 1;
                            break;
                    case 1: // ------------------------
                            // Resolving hostname to IP
                            // ------------------------
                            _client->loop();
                            if (WiFi.hostByName(_mqtt_hostname.c_str(), _mqtt_host_ip) == 1)
                                _mqtt_status = 2;
                            else vTaskDelay(200/portTICK_PERIOD_MS);
                            break;

                    case 2: // --------------------
                            // Connecting to broker
                            // --------------------
                            Serial.println("MQTT-credentials: " + String(_mqtt_hostname) + ":" + String(_mqtt_port));
                            Serial.println("MQTT-credentials: " + String(_mqtt_user + " " + String(_mqtt_passwd + " " + String(_mqtt_use_tls))));
                            Serial.println("MQTT_ID:" + _mqtt_connction_id);
                            if (_mqtt_use_tls)
                                _client->begin(_mqtt_host_ip.toString().c_str(), (int)_mqtt_port, *_net_secure);
                            else
                                _client->begin(_mqtt_host_ip.toString().c_str(), (int)_mqtt_port, *_net_unsec);
                            _mqtt_status = 3;
                            vTaskDelay(100/portTICK_PERIOD_MS);
                            _client->loop();
                            break;

                    case 3: // -----------------------------
                            // Wait for connection to broker
                            // -----------------------------
                            vTaskDelay(1000/portTICK_PERIOD_MS);
                            _client->loop();
                            if (_client->connect(_mqtt_connction_id.c_str(),_mqtt_user.c_str(),_mqtt_passwd.c_str()))
                            {
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
                            if (!_client->connected())
                            {
                                _mqtt_status = 0;
                                mqtt_is_connected = false;
                                if (onDisconnectptr != NULL)
                                    (*onDisconnectptr)();
                            }
                            else
                            {
                                _client->loop();
                                vTaskDelay(10);
                            }
                            break; 
                    default: break;
                  
                    
                }
                
            }
        }
        // constructor
        ConnectingTask(MQTTClient*  client,
                       String&      wifi_ssid, 
                       String&      wifi_passwd, 
                       String&      mqtt_hostname,
                       uint32_t&    mqtt_port,
                       String&      mqtt_user,
                       String&      mqtt_passwd,
                       bool         mqtt_use_tls,
                       WiFiClient*       net_unsec,
                       WiFiClientSecure* net_secure)
        {
            _wifi_status    = 0;
            _mqtt_status    = 0;
            _wifi_ssid      = wifi_ssid;
            _wifi_passwd    = wifi_passwd;
            _mqtt_hostname  = mqtt_hostname;
            _mqtt_port      = mqtt_port;
            _mqtt_user      = mqtt_user;
            _mqtt_passwd    = mqtt_passwd;
            _mqtt_use_tls   = mqtt_use_tls;
            _client         = client;
            _net_secure     = net_secure;
            _net_unsec      = net_unsec;
            
            _mqtt_connction_id = "ESP" + String((uint16_t)(random(0,1000))) + String((uint16_t)(ESP.getEfuseMac()>>32));
            onConnectptr    = NULL;
            onDisconnectptr = NULL;
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
    WifiMQTT(   MQTTClient* client, 
                String     wifi_ssid, 
                String     wifi_passwd, 
                String     mqtt_hostname, 
                uint32_t   mqtt_port,
                String     mqtt_user,
                String     mqtt_passwd,
                bool       mqtt_use_tls); // constructor

    bool wifi_is_connected();
    bool mqtt_is_connected(); 
    bool mqtt_is_secure();   
    void start();
    void stop();
    void set_onConnected(void (*funcptr)(void));  
    void set_onDisconnected(void (*funcptr)(void));   
};
// -------------------------------------------------------------
#endif