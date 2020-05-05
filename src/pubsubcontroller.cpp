#include "pubsubcontroller.h"

//-------------------------------------
void PubSubController::setChannelTopic(int newchannel, String newtopic, int _mode)
//-------------------------------------
{
    boolean needUnsubscribe = false;
    boolean needSubscribe   = false;
    if (newchannel<1 || newchannel>CHANNEL_MAX) return;

    int newtopiclen     = newtopic.length(); 
    int oldtopiclen     = topic[newchannel-1].length();
    String oldtopic     = topic[newchannel-1];
    boolean topicchange = (oldtopic != newtopic);

    topic[newchannel-1] = "";

    //Serial.printf("CHANNEL: %i",newchannel);
    // unsubscribe if topic changed
    if (newtopiclen>0  && oldtopiclen>0 && topicchange) 
    {
        needSubscribe = needUnsubscribe = true;
        //Serial.print("A");

    }
    
    // unsubscribe anyway if newtopic is empty
    if (newtopiclen==0  && oldtopiclen>0) needUnsubscribe = true;
    /*
    // subscribe if newtopic exist but not oldtopic
    if (newtopiclen>0  && oldtopiclen==0) needSubscribe = true;
    */
    // subsribe if new topic exists
    if (newtopiclen>0 && topicchange) needSubscribe = true;
    // but not if an other channel needs the topic or already subscribed

    for (int i = 0;i < CHANNEL_MAX;i++)
    {
        if (topic[i]==oldtopic)
        {
            needUnsubscribe = false;
            //Serial.print("b");
        }
        if (topic[i]==newtopic)
        {
            needSubscribe   = false;
            //Serial.print("B");
        }
    }
     
    if (mqttClient.connected()==true)
    {
        if (needUnsubscribe) 
        {
            mqttClient.unsubscribe(oldtopic.c_str());
            //Serial.print("U");
        }
        if (needSubscribe)   
        {
            mqttClient.subscribe(newtopic.c_str());
            //Serial.print("S");
        }
        topic[newchannel-1] = newtopic;
        state[newchannel-1] = CHANNEL_STATE_DEACTIVE;
        if (newtopic.length()>0)
            state[newchannel-1] = CHANNEL_STATE_ACTIVE;
        if (topicchange)
        {
            data[newchannel-1]  ="";
            last[newchannel-1]  = 0;
            min[newchannel-1]   =  __DBL_MAX__;
            max[newchannel-1]   =  __DBL_MIN__;
            sum[newchannel-1]   = 0;
            count[newchannel-1] = 0;
            mode[newchannel-1]  = _mode;
            msstamp[newchannel-1] = 0;
        }
        
    }
    else
    {
        // connection error handling
    }
}

//------------------------------------------
int PubSubController::getChannelFromTopic(String searchtopic)
//------------------------------------------
{
    for (int i = 0; i < CHANNEL_MAX;i++)
    {
        if (topic[i]==searchtopic) return (i+1);
    }
    
    return -1;
}
//------------------------------------------
boolean PubSubController::setChannelData(String _topic,String _data)
//------------------------------------------
{
    boolean success = false;
    double dbdata = 0.0;
    for (int i = 0; i < CHANNEL_MAX;i++)
    {
        if (topic[i]==_topic) 
        {
            if (cmd[i]==CHANNEL_CMD_CLEAR)
            {
                data[i] = "";
                last[i]  = 0;
                min[i]   =  __DBL_MAX__;
                max[i]   =  __DBL_MIN__;
                sum[i]   = 0;
                count[i] = 0;
                cmd[i]   = CHANNEL_CMD_NOCMD;
            }
            data[i] = _data;
            if (_data.indexOf(".")>=0)
                // we convert to double
                dbdata = _data.toDouble();
            else
                // we convert to int 
                dbdata = _data.toInt();

            last[i] = dbdata;
            if (dbdata>max[i]) max[i]=dbdata;
            if (dbdata<min[i]) min[i]=dbdata;
            sum[i] += dbdata;
            count[i]++;
            msstamp[i] = millis();
            success = true;
        }
    }
    return success;
}
//------------------------------------------
String PubSubController::Excelstream(void)
//------------------------------------------
{
    String result = "";
    int lastchannel = -1;

    // searching for the last channel in activ mode
    for (int i = CHANNEL_MAX-1; i >= 0;i--)
    {
        if (state[i]>=CHANNEL_STATE_ACTIVE) 
        {
            lastchannel = i;
            break;
        }
    }
    if (lastchannel<0) 
    {
        result = "";
        return result;
    }
    for (int i = 0; i <= lastchannel;i++)
    {
        if (state[i]>=CHANNEL_STATE_ACTIVE)
        {
            char s[20];
            switch(mode[i])
            {
                case CHANNEL_MODE_LAST: sprintf(s,"%.2lf",last[i]); break;
                case CHANNEL_MODE_MIN:  sprintf(s,"%.2lf",min[i]);  break;
                case CHANNEL_MODE_MAX:  sprintf(s,"%.2lf",max[i]);  break;
                case CHANNEL_MODE_AVG:  if (count[i]>0)
                                        sprintf(s,"%.2lf",sum[i]/count[i]);
                                        else sprintf(s,"%s","-");
                                        break;
            }
            result += s;//data[i];
            cmd[i] = CHANNEL_CMD_CLEAR;
        }
        if (i==lastchannel) result += "\n";
        else result += ",";    
    }
    return result;
}
//------------------------------------------
void PubSubController::resubscribeAllChannels()
//------------------------------------------
{
    for (int i = 0;i < CHANNEL_MAX;i++)
    {
        if (state[i] && (topic[i].length()>0) && mqttClient.connected())
        {
            mqttClient.subscribe(topic[i].c_str());
        }
    }
}
//--------------------------------------------------------------
double  PubSubController::getChannelData(int channel,int mode )
//--------------------------------------------------------------
{
    double result = INVALID_DATA_VALUE;
    if (channel<1 || channel>CHANNEL_MAX) return result;
    //char s[20];
    switch (mode)
    {
        case CHANNEL_MODE_LAST: result = last[channel-1]; break;
        case CHANNEL_MODE_MAX:  result = max[channel-1]; break;
        case CHANNEL_MODE_MIN:  result = min[channel-1]; break;
        case CHANNEL_MODE_AVG:  result = (sum[channel-1]/count[channel-1]); break;    
    }
    cmd[channel] = CHANNEL_CMD_CLEAR;
    //return String(s);
    return result;
}
//--------------------------------------------
void  PubSubController::clearAllChannelData(void)
//--------------------------------------------
{
    for (int i = 0;i < CHANNEL_MAX;i++)
    {
        data[i] = "";
        last[i]  = 0;
        min[i]   =  __DBL_MAX__;
        max[i]   =  __DBL_MIN__;
        sum[i]   = 0;
        count[i] = 0;
    }  
}
//--------------------------------------------
boolean PubSubController::isNewDataOnAllActiveChannels(uint32_t ms_snap)
//--------------------------------------------
{   
    int lastchannel = -1;
    // searching for the last channel in activ mode
    for (int i = CHANNEL_MAX-1; i >= 0;i--)
    {
        if (state[i]>=CHANNEL_STATE_ACTIVE) 
        {
            lastchannel = i;
            break;
        }
    }
    if (lastchannel<0) return false;
    for (int i = 0; i <= lastchannel;i++ )
    {
        if (state[i] != CHANNEL_STATE_ACTIVE) continue;
        if (msstamp[i]<=ms_snap) return (false);
    }
    return (true);
}
