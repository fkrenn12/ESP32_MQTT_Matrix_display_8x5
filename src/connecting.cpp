
#include "connecting.h"

extern bool connected;   
extern String ssid;
extern String password; 
extern String mqttIP; 
extern int mqttPort;
extern String mqttUser;
extern String mqttPass; 
extern portMUX_TYPE Mutex;
extern TFT_eSPI tft;
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
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_BLACK, TFT_RED); // Set the font colour AND the background colour
    tft.setTextSize(2);
    tft.setCursor(0, 0); // Set cursor at top left of screen
    tft.print("Wifi connect to:\nSSID: ");
    tft.println(_ssid);
    tft.println("PASS: ----------");
    do
    {
      vTaskDelay(10/ portTICK_PERIOD_MS);
      switch (state)
      {
        case 0://
                if (WiFi.status() == WL_CONNECTED)  
                state = 1; 
              timer++;
              if (timer % 100 == 0) tft.print(".");
              if (timer > 1000) 
              {
                tft.println("\nNot connnected!");
                tft.println("Check Wifi\ncredential!");
                connected = false;
                vTaskDelay(2000/ portTICK_PERIOD_MS);
                state = 4;
              }            
              break;
        case 1://
                  client.setServer(_broker_ip, _broker_port);
                  client.setCallback(messageReceived);
                  tft.println("Connect MQTT:");
                  //tft.println(_broker_ip);
                  //tft.println(chipid_str);
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
                tft.println("\nNot connnected!");
                tft.println("Check Broker\nsettings!");
                vTaskDelay(2000/ portTICK_PERIOD_MS);
                connected = false;
                state = 4;
              } 
              break;
        case 3://
              tft.println("\nConnected!");
              tft.fillScreen(TFT_BLACK);
              tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Set the font colour AND the background colour
              vTaskDelay(500/ portTICK_PERIOD_MS);
              tft.fillScreen(TFT_BLACK);
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