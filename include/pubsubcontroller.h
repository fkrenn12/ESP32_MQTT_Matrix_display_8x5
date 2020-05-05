#include "Arduino.h"
#include "ListLib.h"
#include <PubSubClient.h> // MQTT Library
#ifndef _PUBSUBCONTROLLER_H
#define _PUBSUBCONTROLLER_H

#define INVALID_DATA_VALUE      -999999
#define CHANNEL_MAX             20
#define CHANNEL_STATE_DEACTIVE   0
#define CHANNEL_STATE_ACTIVE     1
#define CHANNEL_MODE_LAST        0
#define CHANNEL_MODE_MIN         1
#define CHANNEL_MODE_MAX         2
#define CHANNEL_MODE_AVG         3
#define CHANNEL_CMD_NOCMD        0
#define CHANNEL_CMD_CLEAR        1

class PubSubController 
{
    private:
    PubSubClient& mqttClient;
    public:
    PubSubController(PubSubClient& mqttclient_):mqttClient(mqttclient_)
    {
         for (int i = 0; i < CHANNEL_MAX;i++)
        {
            topic[i]            = "";
            state[i]            = CHANNEL_STATE_DEACTIVE;
            data[i]             = "";
            last[i]             = 0;
            min[i]              = __DBL_MAX__;
            max[i]              = __DBL_MIN__;
            sum[i]              = 0;
            count[i]            = 0;
            mode[i]             = CHANNEL_MODE_LAST;
            cmd[i]              = CHANNEL_CMD_NOCMD;
            msstamp[i]          = 0;
        }
    }
    String   topic[CHANNEL_MAX];
    String   data [CHANNEL_MAX];
    double   last [CHANNEL_MAX];
    double   min  [CHANNEL_MAX];
    double   max  [CHANNEL_MAX];
    double   sum  [CHANNEL_MAX];
    int      count[CHANNEL_MAX];
    int      state[CHANNEL_MAX];
    int      mode [CHANNEL_MAX];
    int      cmd[CHANNEL_MAX];
    uint32_t msstamp[CHANNEL_MAX];
  

    void    setChannelTopic(int,String,int);
    int     getChannelFromTopic(String);
    boolean setChannelData(String,String);
    double  getChannelData(int,int);
    void    clearAllChannelData(void);
    void    resubscribeAllChannels(void);
    boolean isNewDataOnAllActiveChannels(uint32_t ms_snap);
    String  Excelstream(void);
};

#endif