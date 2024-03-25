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

    // actuatorMap is a map of actuator ID to the actuator pin number on the devince.
    // the ID is used to translate the action from server.
    std::map<int, int> actuatorMap;

    // configurationActionsMap is map of action type to the executor function
    // that will be invoked. The json payload will be on form of [[actionType, configurationCategory, configurationValue]]
    std::map<int, void (Usecase::*)(DynamicJsonDocument*)> configurationActionsMap;

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
        char topic[] = "device-initial-state-request/" DEVICE_ID;
        this->repository->PushJSON(topic, NULL);

        // Define the dispatcher map based on the incoming topic
        this->dispatcherMap["d-" DEVICE_ID] = &Usecase::doAction;
        this->dispatcherMap["dcs-" DEVICE_ID] = &Usecase::doInitDeviceCurrentState;
    };

    // doAction will be the entry point of the usecase. It will invoke the
    // executor function based on the given actionType sent from the server.
    void doAction(DynamicJsonDocument* json) {
        int actuatorID = (*json)[0].as<int>();
        if (actuatorMap.find(actuatorID) == actuatorMap.end()) {
            return;
        }

        String value = (*json)[1].as<String>();
        digitalWrite(this->actuatorMap[actuatorID], value == "true" ? LOW : HIGH);

    }

    // this function will be invoked when the device is initialized. It will
    // update the current state of the device based on the given json payload
    // from the server.
    void doInitDeviceCurrentState(DynamicJsonDocument* json) {
        if (json == NULL) {
            return;
        }

        // Register available actuator
        JsonArray actuators = (*json)["a"].as<JsonArray>();
        for (JsonArray item : actuators) {
            int pinNumber = item[1];
            actuatorMap[pinNumber] = item[0];
            pinMode(pinNumber, OUTPUT);
        }

        // Initialize actuator state
        JsonArray states = (*json)["v"].as<JsonArray>();
        for (JsonArray config : states) {
            digitalWrite(this->actuatorMap[config[0]], config[1]);
        }
    }

    void dispatchMQMessage(String topic, DynamicJsonDocument* json) {
        if (dispatcherMap.find(topic) == dispatcherMap.end()) {
            return;
        }

        (this->*dispatcherMap[topic])(json);
    }
};

#endif