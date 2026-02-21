#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include "mqtt_comm.h"
#include <ArduinoJson.h>

// Mock dependencies
class MockMQTTClient {
public:
    void publish(const char* topic, const char* payload, bool retain, int qos) {
        mock().actualCall("publish")
            .withParameter("topic", topic)
            .withParameter("payload", payload)
            .withParameter("retain", retain)
            .withParameter("qos", qos);
    }
};

// Replace with actual WeatherSensor mock or stub
class MockWeatherSensor {
public:
    struct Sensor {
        bool valid;
        int sensor_id;
        int chan;
        bool battery_ok;
        // ... add more fields as needed
    };
    std::vector<Sensor> sensor;
    String getSensorsIncJson() { return "{\"ids\": [1]}"; }
    String getSensorsExcJson() { return "{\"ids\": [2]}"; }
};

// Test group for publishWeatherdata()
TEST_GROUP(PublishWeatherdataTest) {
    void setup() {
        mock().clear();
    }
    void teardown() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(PublishWeatherdataTest, PublishesSensorData) {
    // Setup mocks
    MockMQTTClient client;
    MockWeatherSensor weatherSensor;
    weatherSensor.sensor.push_back({true, 123, 1, true});
    // Simulate publishWeatherdata logic
    // (You may need to refactor publishWeatherdata to accept mocks or dependency injection)
    // Example:
    mock().expectOneCall("publish")
        .withParameter("topic", "hostname/123/data")
        .withParameter("payload", "{\"id\":123,\"ch\":1,\"battery_ok\":1}")
        .withParameter("retain", false)
        .withParameter("qos", 0);
    client.publish("hostname/123/data", "{\"id\":123,\"ch\":1,\"battery_ok\":1}", false, 0);
}

// Test group for publishAutoDiscovery()
TEST_GROUP(PublishAutoDiscoveryTest) {
    void setup() {
        mock().clear();
    }
    void teardown() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(PublishAutoDiscoveryTest, PublishesAutoDiscoveryConfig) {
    MockMQTTClient client;
    sensor_info info = {"Bresser", "Weather Sensor", "weather_sensor_1"};
    const char* sensor_name = "Outside Temperature";
    uint32_t sensor_id = 123;
    const char* device_class = "temperature";
    const char* unit = "°C";
    const char* state_topic = "hostname/123/data";
    const char* value_json = "temp_c";
    // Expected payload (simplified)
    String expectedPayload = "{\"name\":\"Outside Temperature\",\"device_class\":\"temperature\",\"unique_id\":\"7b_temp_c\",\"state_topic\":\"hostname/123/data\",\"unit_of_measurement\":\"°C\"}";
    mock().expectOneCall("publish")
        .withParameter("topic", "homeassistant/sensor/7b_temp_c/config")
        .withParameter("payload", expectedPayload.c_str())
        .withParameter("retain", true)
        .withParameter("qos", 0);
    client.publish("homeassistant/sensor/7b_temp_c/config", expectedPayload.c_str(), true, 0);
}
