#ifndef USECASE_H
#define USECASE_H

#include <String.h>

#include <map>

#include "repository.h"

const int PIN_SOIL_MOISTURE = A0;
const int PIN_RELAY = D0;
const int SOIL_MOISTURE_INTERVAL_MILLIS = 1000 * 60; // 1 minute
class Usecase {
   private:
    Repository* repository;
    int lastSoilMoistureMillis = 0;

    // actionsMap is map of action type to the executor function that will be
    // invoked. The json payload will be on form of [[actionType,
    // actionPayload]] where actionPayload is optional
    std::map<int, void (Usecase::*)(DynamicJsonDocument*)> actionsMap;

    long readSoilMoistureSensor() {
        if (millis() - this->lastSoilMoistureMillis <
            SOIL_MOISTURE_INTERVAL_MILLIS) {
            return -1;
        }

        this->lastSoilMoistureMillis = millis();
        int value = analogRead(PIN_SOIL_MOISTURE);

        return value;
    }

    void handleToggleRelay(DynamicJsonDocument* json) {
        String relayState = (*json)[1].as<String>();
        digitalWrite(PIN_RELAY, relayState == "true" ? HIGH : LOW);
    }

   public:
    virtual ~Usecase(){};
    Usecase(){};
    Usecase(Repository* repository) : repository(repository) {
        pinMode(LED_BUILTIN, OUTPUT);
        pinMode(PIN_RELAY, OUTPUT);
        pinMode(A0, INPUT);

        // Define actions map based on the given first array element of the
        // request
        this->actionsMap[1] = &Usecase::handleToggleRelay;
    };

    // doAction will be the entry point of the usecase. It will invoke the
    // executor function based on the given actionType sent from the server.
    void doAction(DynamicJsonDocument* json) {
        Serial.println(json->as<String>());
        int action = (*json)[0].as<int>();

        // If actions is not found, simply fallback to default action
        // Which is to turn on/off the board LED
        if (actionsMap.find(action) == actionsMap.end()) {
            String state = (*json)[1].as<String>();
            digitalWrite(LED_BUILTIN, state == "true" ? LOW : HIGH);
            return;
        }

        (this->*actionsMap[action])(json);
    }

    void readSensor() {
        DynamicJsonDocument doc(1024);
        int currentIndex = -1;
        long soilMoisture = readSoilMoistureSensor();
        if (soilMoisture != -1) {
            currentIndex++;
            doc[currentIndex][0] = 3;
            doc[currentIndex][1] = soilMoisture;
        }

        if (currentIndex < 0) {
            return;
        }

        repository->PushJSON("sensor-read/" DEVICE_ID, &doc);
    };
};

#endif