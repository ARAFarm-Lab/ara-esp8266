#ifndef USECASE_H
#define USECASE_H

#include <String.h>

#include <map>

#include "repository.h"

const int SOIL_MOISTURE_PIN = A0;
const int SOIL_MOISTURE_INTERVAL_MILLIS = 1000;
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
        int value = analogRead(SOIL_MOISTURE_PIN);

        return value;
    }

    void handleToggleRelay(DynamicJsonDocument* json) {
        // TODO: Implement this
    }

   public:
    virtual ~Usecase(){};
    Usecase(){};
    Usecase(Repository* repository) : repository(repository) {
        // Define actions map based on the given first array element of the
        // request
        this->actionsMap[1] = &Usecase::handleToggleRelay;
    };

    // doAction will be the entry point of the usecase. It will invoke the
    // executor function based on the given actionType sent from the server.
    void doAction(DynamicJsonDocument* json) {
        int action = (*json)[0][0].as<int>();

        // If actions is not found, simply fallback to default action
        // Which is to turn on/off the board LED
        if (actionsMap.find(action) == actionsMap.end()) {
            digitalWrite(LED_BUILTIN, (*json).as<String>() == "true" ? LOW : HIGH);
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