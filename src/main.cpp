#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <MAX6675.h>

const char* ssid = "ESP32-thermocouple";
const char* password = "1234567890";

const char* csvName = "/temps.csv";

WebServer server(80);


int thermoDO = 19;
int thermoCS = 5;
int thermoCLK = 18;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);


void writeCsvHeader() {
    if (!SPIFFS.exists(csvName)) {
        File file = SPIFFS.open(csvName, FILE_WRITE);
        if (file) {
            file.println("time_ms, temp_C \n");
            file.close();
        }
    }
}

void logTemperatureCSV() {
    File file = SPIFFS.open(csvName, FILE_APPEND);

    if (!file) {
        Serial.println("Failed to open csv for appending");
        return;
    }

    float temp = thermocouple.readCelsius();

    unsigned long timestamp = millis();

    file.printf("%lu, %.2f \n", timestamp, temp);

    file.close();

}    

void setup() {
    Serial.begin(115200);
    delay(1000);

    // connect to existing wifi network
    // WiFi.begin(ssid, password);
    // Serial.print("connecting to wifi");
    // while (Wifi.status() != WL_CONNECTED) {
    //     delay(500);
    //     Serial.print(".");
    // }

    // create access point
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access point initiated. Visit http://");
    Serial.println(IP);

    
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return;
    }
    Serial.println("SPIFFS mount successful");

    writeCsvHeader();

    server.on("/download", HTTP_GET, []() {
        File file= SPIFFS.open(csvName, "r");
        if (!file) {
            server.send(500, "text/plain", "Failed to open file");
            return;
        }

        server.streamFile(file, "text/csv");
        file.close();
    });

    server.begin();
}


void loop() {
    server.handleClient();

    static unsigned long lastLog = 0;
    if (millis() - lastLog > 5000) {
        logTemperatureCSV();
        lastLog = millis();
    }
}