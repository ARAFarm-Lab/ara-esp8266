#ifndef USECASE_H
#define USECASE_H

#include <String.h>

#include <map>

#include "constants.h"
#include "repository.h"

const int PIN_SOIL_MOISTURE = A0;
const int PIN_RELAY = D0;
class Usecase {
   private:
    Repository* repository;

    // soil moisturizer sensor configuration
    int lastSoilMoistureMillis = 0;
    int SOIL_MOISTURE_INTERVAL_MILLIS = 1000 * 60;  // 1 minute
    bool isSoilMoistureSensorEnabled = false;       // by default is set to false

    // dispatcherMap is map of topic to the executor function that will be
    std::map<String, void (Usecase::*)(DynamicJsonDocument*)> dispatcherMap;

    // actionsMap is map of action type to the executor function that will be
    // invoked. The json payload will be on form of [[actionType, actionPayload]]
    std::map<int, void (Usecase::*)(DynamicJsonDocument*)> actionsMap;

    // configurationActionsMap is map of action type to the executor function
    // that will be invoked. The json payload will be on form of [[actionType, configurationCategory, configurationValue]]
    std::map<int, void (Usecase::*)(DynamicJsonDocument*)> configurationActionsMap;

    // doAction will be the entry point of the usecase. It will invoke the
    // executor function based on the given actionType sent from the server.
    void doAction(DynamicJsonDocument* json) {
        int action = (*json)[0].as<int>();

        if (actionsMap.find(action) == actionsMap.end()) {
            Serial.println("action not found");
            return;
        }

        (this->*actionsMap[action])(json);
    }

    // doConfigurationAction will be the entry point of the usecase. It will
    // invoke the executor function based on the given actionType sent from the
    // server.
    void doConfigurationAction(DynamicJsonDocument* json) {
        int action = (*json)[0].as<int>();

        // If actions is not found, simply fallback to default action
        // Which is to turn on/off the board LED
        if (configurationActionsMap.find(action) ==
            configurationActionsMap.end()) {
            String state = (*json)[1].as<String>();
            digitalWrite(LED_BUILTIN, state == "true" ? LOW : HIGH);
            return;
        }

        (this->*configurationActionsMap[action])(json);
    }

    // this function will be invoked when the device is initialized. It will
    // update the current state of the device based on the given json payload
    // from the server.
    void doInitDeviceCurrentState(DynamicJsonDocument* json) {
        if (json == NULL) {
            return;
        }

        if ((*json).as<JsonArray>().size() == 0) {
            return;
        }

        for (JsonArray config : (*json).as<JsonArray>()) {
            int actionType = config[0].as<int>();
            if (actionType == ACTION_TYPE_BUILT_IN_LED) {
                String state = config[1].as<String>();
                digitalWrite(LED_BUILTIN, state == "true" ? LOW : HIGH);
                continue;
            }

            if (actionType == ACTION_TYPE_TOGGLE_RELAY) {
                String state = config[1].as<String>();
                digitalWrite(PIN_RELAY, state == "true" ? LOW : HIGH);
                continue;
            }
        }
    }

    void handleToggleBuiltInLED(DynamicJsonDocument* json) {
        String ledState = (*json)[1].as<String>();
        digitalWrite(LED_BUILTIN, ledState == "true" ? LOW : HIGH);
    }

    void handleToggleRelay(DynamicJsonDocument* json) {
        String relayState = (*json)[1].as<String>();
        digitalWrite(PIN_RELAY, relayState == "true" ? LOW : HIGH);
    }

    long readSoilMoistureSensor() {
        if (!this->isSoilMoistureSensorEnabled) {
            return -1;
        }

        if (millis() - this->lastSoilMoistureMillis <
            SOIL_MOISTURE_INTERVAL_MILLIS) {
            return -1;
        }

        this->lastSoilMoistureMillis = millis();
        int value = analogRead(PIN_SOIL_MOISTURE);

        return value;
    }

    // updateSoilMoistureSensorConfiguration will update the configuration of
    // the soil moisture sensor. The json payload will be on form of
    // [[actionType, configurationCategory, configurationValue]]
    void updateSoilMoistureSensorConfiguration(DynamicJsonDocument* json) {
        int configCategory = (*json)[1].as<int>();
        if (configCategory == CONFIG_SOIL_MOISTURE_ENABLE) {
            bool isEnabled = (*json)[2].as<bool>();
            this->isSoilMoistureSensorEnabled = isEnabled;
            return;
        }

        if (configCategory == CONFIG_SOIL_MOISTURE_INTERVAL) {
            int interval = (*json)[2].as<int>();
            this->SOIL_MOISTURE_INTERVAL_MILLIS = interval;
            return;
        }
    }

   public:
    virtual ~Usecase(){};
    Usecase(){};
    Usecase(Repository* repository) : repository(repository) {
        pinMode(LED_BUILTIN, OUTPUT);
        pinMode(PIN_RELAY, OUTPUT);
        pinMode(A0, INPUT);

        // Flush all the pins
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(PIN_RELAY, HIGH);

        // Push a request to the server to get the initial state of the device
        // The server will return with another message to update the device
        this->repository->PushJSON("device-initial-state-request/" DEVICE_ID, NULL);

        // Define the dispatcher map based on the incoming topic
        this->dispatcherMap["d-" DEVICE_ID] = &Usecase::doAction;
        this->dispatcherMap["s-" DEVICE_ID] = &Usecase::doConfigurationAction;
        this->dispatcherMap["dcs-" DEVICE_ID] = &Usecase::doInitDeviceCurrentState;

        // Define actions map based on the given first array element of the request
        this->actionsMap[ACTION_TYPE_BUILT_IN_LED] = &Usecase::handleToggleBuiltInLED;
        this->actionsMap[ACTION_TYPE_TOGGLE_RELAY] = &Usecase::handleToggleRelay;

        // Define configuration actions map based on the given first array element of the request
        this->actionsMap[CONFIG_TYPE_SOIL_MOISTURE] = &Usecase::updateSoilMoistureSensorConfiguration;
    };

    void dispatchMQMessage(String topic, DynamicJsonDocument* json) {
        if (dispatcherMap.find(topic) == dispatcherMap.end()) {
            return;
        }

        (this->*dispatcherMap[topic])(json);
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

        this->repository->PushJSON("sensor-read/" DEVICE_ID, &doc);
    };
};

#endif