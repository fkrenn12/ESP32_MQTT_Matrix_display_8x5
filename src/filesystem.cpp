#include <filesystem.h>

// --------------------------
bool InitalizeFileSystem(bool print_status)
// --------------------------
{
  bool initok = false;
  initok = SPIFFS.begin();
  if (!(initok)) // Format SPIFS, of not formatted. - Try #1
  {
    if (print_status)  
        Serial.println("SPIFFS Dateisystem formatiert.");
    SPIFFS.format();
    initok = SPIFFS.begin();
  }
  if (!(initok)) // Format SPIFS. - Try #2
  {
    SPIFFS.format();
    initok = SPIFFS.begin();
  }
  if (print_status)
    if (initok) 
    { Serial.println("SPIFFS ist  OK"); } 
    else 
    { Serial.println("SPIFFS ist nicht OK"); }
  return initok;
}