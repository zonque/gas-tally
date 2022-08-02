#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <ESP8266WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "config.h"

#define PIN_LED D4

static Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(0);

static ESP8266WiFiMulti wifiMulti;

static InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

static bool latched = false;

void displaySensorDetails(void)
{
    sensor_t sensor;
    mag.getSensor(&sensor);

    Serial.println("------------------------------------");

    Serial.print("Sensor:       ");
    Serial.println(sensor.name);

    Serial.print("Driver Ver:   ");
    Serial.println(sensor.version);

    Serial.print("Unique ID:    ");
    Serial.println(sensor.sensor_id);

    Serial.print("Max Value:    ");
    Serial.print(sensor.max_value);
    Serial.println(" uT");

    Serial.print("Min Value:    ");
    Serial.print(sensor.min_value);
    Serial.println(" uT");

    Serial.print("Resolution:   ");
    Serial.print(sensor.resolution);
    Serial.println(" uT");

    Serial.println("------------------------------------");
}

static void logDataPoint()
{
    static Point sensor("usage");

    sensor.clearFields();
    sensor.clearTags();

    sensor.addTag("sensor-id", DATA_POINT_SENSOR_ID);
    sensor.addField("m3", DATA_POINT_VALUE);

    if (!client.writePoint(sensor)) {
        Serial.print("InfluxDB write failed: ");
        Serial.println(client.getLastErrorMessage());
    }
}

void setup(void)
{
    delay(3000);

    Serial.begin(115200);
    Serial.println("Gas Tally");
    Serial.println("");

    mag.begin();
    mag.setMagGain(HMC5883_MAGGAIN_8_1);

    displaySensorDetails();

    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wifi SSID ");
    Serial.print(WIFI_SSID);

    while (wifiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println("");

    timeSync(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2);

    if (client.validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());
    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
    }

    client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));

    pinMode(PIN_LED, OUTPUT);
}

void loop(void)
{
    digitalWrite(PIN_LED, 1);

    sensors_event_t event;
    mag.getEvent(&event);

    Serial.print("X: ");
    Serial.print(event.magnetic.x);
    Serial.print("  ");

    Serial.print("Y: ");
    Serial.print(event.magnetic.y);
    Serial.print("  ");

    Serial.print("Z: ");
    Serial.print(event.magnetic.z);
    Serial.print("  ");

    Serial.println("uT");

    int value = event.magnetic.z;

    if (value > MAGNET_THRESHOLD_HIGH && latched) {
        Serial.println("Turnaround detected, logging data point!");
        logDataPoint();
        latched = false;
        digitalWrite(PIN_LED, 0);
    }

    if (value < MAGNET_THRESHOLD_LOW)
        latched = true;

    delay(1000);
}