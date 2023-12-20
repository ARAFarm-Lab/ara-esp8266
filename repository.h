#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
#include <ESP8266WiFi.h>

#include "config.h"

class Repository {
    MqttClient* mq;

   public:
    virtual ~Repository(){};
    Repository(){};
    Repository(MqttClient* mq) : mq(mq){};

    void PushJSON(DynamicJsonDocument* json) {
        this->mq->beginMessage(MQ_TOPIC);
        this->mq->print(serializeJson(*json, Serial));
        this->mq->endMessage();
    };
};

#endif