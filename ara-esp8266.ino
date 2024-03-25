#include <ArduinoMqttClient.h>
#include <ESP8266WiFi.h>
#include <String.h>

#include "config.h"
#include "repository.h"
#include "usecase.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// Layers
Repository repository;
Usecase usecase;

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing Device");

    // Turn on the LED indicating the device is initializing
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize external dependencies
    connectToWifi();
    connectToMQBroker();

    // Initialize Layers
    repository = Repository(&mqttClient);
    usecase = Usecase(&repository);

    // Initialize MQ message handler
    mqttClient.onMessage(handleMQIncomingMessage);

    // Turn off the LED indicating the device is initializing
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Initialization Success");
}

void loop() {
    // Check the wifi connection every loop iteration
    // Reconnect if it is disconnected.
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wifi disconnected. Trying to reconnect.");
        connectToWifi();
        return;
    }

    // Check the MQ connection every loop iteration
    // Reconnect if it is disconnected.
    if (!mqttClient.connected()) {
        connectToMQBroker();
        return;
    }

    mqttClient.poll();
}

void connectToMQBroker() {
    mqttClient.setId("esp8266-" DEVICE_ID);
    Serial.println("Connecting to MQ Broker");

    while (!mqttClient.connect(MQ_SERVER, MQ_PORT)) {
        Serial.println("Failed to connect to MQ Broker, retrying ....");
        delay(1000);
    }

    mqttClient.setKeepAliveInterval(MQ_KEEP_ALIVE_INTERVAL * 1000L);
    mqttClient.subscribe("d-" DEVICE_ID, MQ_QOS);    // Subscribe to topic that handles device actions
    mqttClient.subscribe("s-" DEVICE_ID, MQ_QOS);    // Subscribe to topic that handles configuration
    mqttClient.subscribe("dcs-" DEVICE_ID, MQ_QOS);  // Subscribe to topic that handles configuration
    Serial.println("Connected to MQ Broker");
}

void connectToWifi() {
    Serial.println("Connecting to Wifi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to Wifi, retrying ....");
        delay(1000);
    }

    Serial.println("Connected to Wifi");
}

void handleMQIncomingMessage(int messageSize) {
    String topic = mqttClient.messageTopic();
    char* message = new char[messageSize + 1];

    int index = 0;
    while (mqttClient.available()) {
        message[index++] = (char)mqttClient.read();
    }
    message[index] = '\0';

    DynamicJsonDocument json(256);
    DeserializationError error = deserializeJson(json, message);
    if (error) {
        return;
    }

    usecase.dispatchMQMessage(topic, &json);
}