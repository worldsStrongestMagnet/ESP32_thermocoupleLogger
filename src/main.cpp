#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <MAX6675.h>

const char* ssid = "AM_ESP32-thermocouple1";
const char* password = "1234567890";
unsigned long loggingInterval = 5000;
unsigned long loggingDuration = 10000UL * 5000UL;
unsigned long logStartTime = 0;
static bool loggerOn = false;
static float currentTemp = 0;

const char* csvName = "/temps.csv";

WebServer server(80);

// thermocoule 1
int thermoDO1 = 19;
int thermoCS1 = 5;
int thermoCLK1 = 18;

MAX6675 thermocouple1(thermoCLK1, thermoCS1, thermoDO1);

// thermocouple 2
int thermoDO2 = 25; 
int thermoCS2 = 33;
int thermoCLK2 = 32;

MAX6675 thermocouple2(thermoCLK2, thermoCS2, thermoDO2);


void writeCsvHeader() {
    if (!SPIFFS.exists(csvName)) {
        File file = SPIFFS.open(csvName, FILE_WRITE);
        if (file) {
            file.println("time_ms, temp_C_1, temp_C_2 \n");
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

    float temp1 = thermocouple1.readCelsius();
    float temp2 = thermocouple2.readCelsius();

    unsigned long timestamp = millis();

    file.printf("%lu, %.2f, %.2f \n", timestamp, temp1, temp2);

    file.close();

}

void clearLog() {
  if (SPIFFS.exists(csvName)) {
    SPIFFS.remove(csvName);
    writeCsvHeader();
  }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

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

    server.on("/", HTTP_GET, []() {
      File file = SPIFFS.open("/index.html", "r");

      if (!file) {
        server.send(500, "text/plain", "file not found");
        return;
      }
      server.streamFile(file, "text/html");
      file.close();
    });

    server.on("/main.js", HTTP_GET, [](){
      File file = SPIFFS.open("/main.js", "r");

      if (!file) {
        server.send(500, "text/plain", "file not found");
        return;
      }

      server.streamFile(file, "text/javascript");
      file.close();
    });

    server.on("/latest", HTTP_GET, []() {
      server.send(200, "text/plain", String(thermocouple1.readCelsius(), 2)); // need to update to send both thermocouple readings
    });

    server.on("/update-log-specs", HTTP_POST, []() {
      if (server.hasArg("interval") && server.hasArg("duration")) {
        loggingInterval = server.arg("interval").toInt();
        loggingDuration = server.arg("duration").toInt();

        server.send(200, "text/plain", "params updated");
      } else {
        server.send(400, "text/plain", "missing params");
      }
    });


    server.on("/download", HTTP_GET, []() {
        File file= SPIFFS.open(csvName, "r");
        if (!file) {
            server.send(500, "text/plain", "Failed to open file");
            return;
        }

        server.streamFile(file, "text/csv");
        file.close();
    });

    server.on("/initiate", HTTP_POST, []() {
      if (server.hasArg("startlog") && server.arg("startlog").toInt() == 1) {
        loggerOn = true;
        logStartTime = millis();

        server.send(200, "text/plain", "logging initiated");
      } else {
        server.send(400, "text/plain", "parameter error - logging not initiated");
      }
    });

    server.on("/clear-log", HTTP_POST, [](){
      if (server.hasArg("clearlogs") && server.arg("clearlogs").toInt() == 1) {
        clearLog();
        server.send(200, "text/plain", "log cleared");
        loggerOn = false;
      } else {
        server.send(400, "text/plain", "parameter error - logs not cleared");
      }
    });

    server.begin();
}


void loop() {
    server.handleClient();

    static unsigned long lastLog = 0;
    // static unsigned long lastRead = 0;
    unsigned long currentTime = millis();

    if (currentTime - logStartTime < loggingDuration && loggerOn) {
      if (currentTime - lastLog >= loggingInterval) {
        logTemperatureCSV();
        lastLog = millis();
      }
    }

    // for a server side approach
    // if (currentTime - logStartTime > loggingDuration) {
    //   loggerOn = false;
    // }

    // if (currentTime - lastRead > 500) {
    //   currentTemp = thermocouple1.readCelsius();
    // }
}