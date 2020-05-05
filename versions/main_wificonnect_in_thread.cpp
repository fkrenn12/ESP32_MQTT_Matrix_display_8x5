#include <WiFi.h>
void wifi(void *pvParameters);
TaskHandle_t wifi_connect;
void setup() 
{
  Serial.begin(250000);
  xTaskCreate(wifi, "wifi connection", 4096, NULL, 1, &wifi_connect);
}

void loop() {
  delay(2000);
}

void wifi(void *pvParameters) 
{
  const char* ssid = "LAWIG14";
  const char* pass = "wiesengrund14";
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED ) {
    vTaskDelay(500/ portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address is ");
  Serial.println(WiFi.localIP());
  for ( ;; ) 
  {
    vTaskDelay(500/ portTICK_PERIOD_MS);
  }
}