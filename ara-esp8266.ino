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
    Serial.begin(9600);

    Serial.println("Initializing Device");

    // Initialize pins
    pinMode(LED_BUILTIN, OUTPUT);

    // Flush all LED
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize external dependencies
    connectToWifi();
    connectToMQBroker();

    // Initialize Layers
    repository = Repository(&mqttClient);
    usecase = Usecase(&repository);

    // Initialize MQ message handler
    mqttClient.onMessage(handleMQIncomingMessage);

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
    mqttClient.setId(MQ_CLIENT_ID);
    Serial.println("Connecting to MQ Broker");

    while (!mqttClient.connect(MQ_SERVER, MQ_PORT)) {
        Serial.println("Failed to connect to MQ Broker, retrying ....");
        delay(1000);
    }

    mqttClient.setKeepAliveInterval(MQ_KEEP_ALIVE_INTERVAL * 1000L);
    mqttClient.subscribe(MQ_TOPIC, MQ_QOS);
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
    String message = "";
    while (mqttClient.available()) {
        message += (char)mqttClient.read();
    }

    Serial.println("Received message: " + message);
    digitalWrite(LED_BUILTIN, message == "true" ? LOW : HIGH);
}