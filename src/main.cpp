#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <MAX6675.h>
#include <ESPmDNS.h>

// const char* ssid = "AM_ESP32-thermocouple";
// const char* password = "1234567890";

const char* ssid     = "Allied Maker";
const char* password = "blackenedbrass";

unsigned long loggingInterval = 5000;
unsigned long loggingDuration = 10000UL * 5000UL;
unsigned long logStartTime = 0;
static bool loggerOn = false;
static float currentTemp = 0;

const char* csvName = "/temps.csv";

WebServer server(80);

// -- thermocouple 1 -- //
int thermoDO = 19;
int thermoCS = 18; //5;
int thermoCLK = 5; //18;

MAX6675 thermocouple1(thermoCLK, thermoCS, thermoDO);

// -- thermocouple 2 -- //
int thermoDO2 = 17;
int thermoCS2 = 16;
int thermoCLK2 = 4;

MAX6675 thermocouple2(thermoCLK2, thermoCS2, thermoDO2);

// struct to hold both thermocouple readings
struct Temps {
  float t1;
  float t2;
};

// function to read both sensors without interfering with one another
Temps serializedSensorRead() {
  Temps out{NAN, NAN};

  // read thermocouple 1 //
  digitalWrite(thermoCS2, HIGH);
  delayMicroseconds(10);
  digitalWrite(thermoCS, LOW);
  delayMicroseconds(5);
  out.t1 = thermocouple1.readCelsius();
  digitalWrite(thermoCS, HIGH);

  delay(5);

  // read thermocouple 2 //
  digitalWrite(thermoCS, HIGH);
  delayMicroseconds(10);
  digitalWrite(thermoCS2, LOW);
  delayMicroseconds(5);
  out.t2 = thermocouple2.readCelsius();
  digitalWrite(thermoCS2, HIGH); 

  delay(5);

  return out;
}

void writeCsvHeader() {
    if (!SPIFFS.exists(csvName)) {
        File file = SPIFFS.open(csvName, FILE_WRITE);
        if (file) {
            file.println("time_ms, temp1_C, temp2_C \n");
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

    // float temp = thermocouple1.readCelsius();
    Temps temp = serializedSensorRead();

    unsigned long timestamp = millis();

    file.printf("%lu, %.2f, %.2f \n", timestamp, temp.t1, temp.t2);

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

    // // create access point
    // WiFi.softAP(ssid, password);
    // IPAddress IP = WiFi.softAPIP();
    // Serial.print("Access point initiated. Visit http://");
    // Serial.println(IP);

  // ——— CONNECT TO LOCAL WIFI ———
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  // ————————————————————————

  // ——— START mDNS RESPONDER ———
  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up mDNS responder!");
  } else {
    Serial.println("mDNS responder started: http://esp32.local");
    MDNS.addService("http", "tcp", 80);
  }

    
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
      Temps tt = serializedSensorRead();
      String json = "{\"temp1\": " + String(tt.t1, 2) + ", \"temp2\": " + String(tt.t2, 2) + "}"; 
      // String json = "{\"temp1\": " + String(thermocouple1.readCelsius(), 2) + "}";
      server.send(200, "application/json", json);

      // server.send(200, "text/plain", String(thermocouple1.readCelsius(), 2));
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
        // Serial.println();
      }
    }

    // for a server side approach
    // if (currentTime - logStartTime > loggingDuration) {
    //   loggerOn = false;
    // }

    // if (currentTime - lastRead > 500) {
    //   currentTemp = thermocouple.readCelsius();
    // }
}