
#include "connecting.h"

extern bool connected;   
extern String ssid;
extern String password; 
extern String mqttIP; 
extern int mqttPort;
extern String mqttUser;
extern String mqttPass; 
extern portMUX_TYPE Mutex;
extern PubSubClient client;
extern char chipid_str[13];
extern void messageReceived(char*, byte*, unsigned int);

//--------------------------------------------------
void connectTask( void * pvParameters ) // Connecting to Wifi and MQTT-Broker
//--------------------------------------------------
{
  #define MAX_LEN 32
  char _ssid[MAX_LEN + 1];
  char _pass[MAX_LEN + 1];
  char _broker_ip[15];
  char _broker_user[MAX_LEN + 1];
  char _broker_pass[MAX_LEN + 1];
  int  _broker_port;

  int state   = 0;
  int timer   = 0;

  for(;;)
  {
    connected = false;
    WiFi.disconnect();
    state = 0;
    timer = 0;
    vTaskEnterCritical(&Mutex);
    strcpy(_ssid,ssid.c_str());
    strcpy(_pass,password.c_str());
    strcpy(_broker_ip,mqttIP.c_str());
    strcpy(_broker_user,mqttUser.c_str());
    strcpy(_broker_pass,mqttPass.c_str());
    _broker_port = mqttPort;
    vTaskExitCritical(&Mutex);
    // Wifi
    WiFi.begin(_ssid, _pass);
    do
    {
      vTaskDelay(10/ portTICK_PERIOD_MS);
      switch (state)
      {
        case 0://
              if (WiFi.status() == WL_CONNECTED)  
                state = 1; 
              timer++;
              if (timer > 1000) 
              {
                connected = false;
                vTaskDelay(2000/ portTICK_PERIOD_MS);
                state = 4;
              }            
              break;
        case 1://
                  client.setServer(_broker_ip, _broker_port);
                  client.setCallback(messageReceived);
                  timer = 0;
                  state = 2;
              break;
        case 2://
                client.setServer(_broker_ip, _broker_port);
                if (client.connect(chipid_str, _broker_user, _broker_pass)) // this function blocks a long time
                {
                  //client.subscribe("/dummy"); // dummy subscription
                  state = 3;          
                }
              timer++;
              if (timer > 5) 
              {
                vTaskDelay(2000/ portTICK_PERIOD_MS);
                connected = false;
                state = 4;
              } 
              break;
        case 3://
              vTaskDelay(500/ portTICK_PERIOD_MS);
              connected = true;
              state = 4;
              break;
        case 4://
              break;
        default:
              break;
      }
    }while(state != 4);
    if (connected) vTaskSuspend(NULL);
    vTaskDelay(500/ portTICK_PERIOD_MS);
  }
}