#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WebServer.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Pins for LEDs
const int redLedPin = 2;
const int blueLedPin = 5;
TinyGPSPlus gps;

float latitude = <example latitude value>; // Example Latitude
float longitude = <example longitude value>; // Example Longitude

String getAPIResponse(String query);
HardwareSerial gpsSerial(1);
#define RX_PIN 16 // ESP32 RX connected to GPS TX
#define TX_PIN 17 // ESP32 TX connected to GPS RX

// WiFi credentials
const char* ssid = "<ssid>";
const char* password = "<password>";
String apiKey = "<api-key-for>";
String endpoint = "https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=" + apiKey;
String GMAP_API_KEY = "<gmap-api-key>";

// MPU6050 object
Adafruit_MPU6050 mpu;

// Web server object
WebServer server(80);

// Variables for inactivity detection
unsigned long lastActiveTime = 0;
bool isResting = false;
const float activityThresholdMax = 10.7; // Threshold for detecting movement
const float activityThresholdMin = 8.5;
const unsigned long inactivityTimeLimit = 10000; // 10 seconds in milliseconds

// Variables for query and response
String userQuery = "";
String apiResponse = "";

// Variables for activity chart
String activityData = ""; // Stores the activity state over time
unsigned long lastChartUpdate = 0;
const unsigned long chartUpdateInterval = 1000; // Update chart every second

// HTML content for the webpage
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>IntelliCollar Animal Health Tracker</title>
  <style>
    body { font-family: Arial; text-align: center;background-color: #79BAEC;}
    h2 { font-size: 2.5rem; }
    p { font-size: 1.5rem; }
    input, button { font-size: 1.2rem; padding: 10px; margin: 10px; }
    .result { margin-top: 20px; font-size: 1.2rem; }
    #map { height: 400px; width: 100%; margin-top: 20px; }
    canvas { margin-top: 20px; }
  </style>
  <script>
    function fetchMPUData() {
      fetch('/mpu').then(response => response.json()).then(data => {
        document.getElementById('mpuX').textContent = data.x.toFixed(2);
        document.getElementById('mpuY').textContent = data.y.toFixed(2);
        document.getElementById('mpuZ').textContent = data.z.toFixed(2);
        document.getElementById('activityStatus').textContent = data.status;
      }).catch(error => console.log('Error fetching MPU data:', error));
    }

    function fetchGPSData() {
      fetch('/gps').then(response => response.json()).then(data => {
        document.getElementById('gpsLat').textContent = data.latitude.toFixed(6);
        document.getElementById('gpsLng').textContent = data.longitude.toFixed(6);
        updateMap(data.latitude, data.longitude);
      }).catch(error => console.log('Error fetching GPS data:', error));
    }

    function fetchChartData() {
      fetch('/chart').then(response => response.json()).then(data => {
        updateChart(data.activity);
      }).catch(error => console.log('Error fetching chart data:', error));
    }

    var map;
    var marker;
    var chart;
    var chartData = [];

    function initMap() {
      var options = {
        zoom: 16,
        center: { lat: parseFloat(document.getElementById('gpsLat').textContent), lng: parseFloat(document.getElementById('gpsLng').textContent) },
        mapTypeId: google.maps.MapTypeId.ROADMAP
      };
      map = new google.maps.Map(document.getElementById('map'), options);
      marker = new google.maps.Marker({
        position: options.center,
        map: map
      });
    }

    function updateMap(lat, lng) {
      var newLatLng = new google.maps.LatLng(lat, lng);
      marker.setPosition(newLatLng);
      map.setCenter(newLatLng);
    }

    function initChart() {
      var ctx = document.getElementById('activityChart').getContext('2d');
      chart = new Chart(ctx, {
        type: 'bar',
        data: {
          labels: [],
          datasets: [{
            label: 'Activity State',
            data: [],
            backgroundColor: 'rgba(0, 50, 50, 0.8)',
            borderColor: 'rgba(0, 100, 100, 1)',

            borderWidth: 1
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            x: {
              title: { display: true, text: 'Time (s)' },
              ticks: { maxTicksLimit: 30 }
            },
            y: {
              title: { display: true, text: 'State (0: Resting, 1: Active)' },
              min: 0,
              max: 1
            }
          }
        }
      });
    }

    function updateChart(activityData) {
      var now = new Date();
      var timeLabel = now.getSeconds();
      chart.data.labels.push(timeLabel);
      chart.data.datasets[0].data.push(activityData);

      if (chart.data.labels.length > 30) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
      }

      chart.update();
    }

    setInterval(fetchMPUData, 1000); // Refresh MPU data every second
    setInterval(fetchGPSData, 1000); // Refresh GPS data every second
    setInterval(fetchChartData, 1000); // Refresh chart data every second
  </script>
</head>
<body onload="initChart()">
  <h2 style="color:black;">IntelliCollar Smart Animal Health Tracker<span class="emoji">🐕‍🦺</span></h2>
  <form action="/submit" method="GET">
    <input type="text" name="query" placeholder="Enter the Animals Name" required>
    <button type="submit">Submit</button>
  </form>
  <div class="result">
    <h3 style="color:orange;">Response:</h3>
    <p>%s</p>
    <h3>MPU6050 Data:</h3>
    <p>X: <span id="mpuX" >0.00</span> m/s²</p>
    <p>Y: <span id="mpuY">0.00</span> m/s²</p>
    <p>Z: <span id="mpuZ">0.00</span> m/s²</p>
    <p>Status: <span id="activityStatus">Unknown</span></p>
    <h3>GPS Data:</h3>
    <p>Latitude: <span id="gpsLat">%f</span></p>
    <p>Longitude: <span id="gpsLng">%f</span></p>
    <h3>Google Map:</h3>
    <div id="map"></div>
    <h3>Activity Chart:</h3>
    <div style="height: 400px; width: 100%;">
      <canvas id="activityChart"></canvas>
    </div>
  </div>
  <script async defer src="https://maps.googleapis.com/maps/api/js?key=%s&callback=initMap"></script>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</body>
</html>
)rawliteral";

// Heart Rate Sensor Variables
#define HEART_RATE_SENSOR_PIN 25  // GPIO pin connected to the XD58C sensor

unsigned long previousBeatTime = 0;  // Time of the last detected beat
unsigned long currentBeatTime = 0;   // Time of the current beat
int bpm = 0;                         // Beats per minute
int beatCount = 0;                   // Count of beats in a given time
unsigned long lastCalculationTime = 0;  // Last time BPM was calculated
const unsigned long bpmCalculationInterval = 10000; // Calculate BPM every 10 seconds

// Function to handle the root page
void handleRoot() {
    char response[5000];
    snprintf(response, sizeof(response), htmlPage, apiResponse.c_str(), latitude, longitude, GMAP_API_KEY.c_str());
    server.send(200, "text/html", response);
}

// Function to handle form submission
void handleFormSubmit() {
    if (server.hasArg("query")) {
        userQuery = server.arg("query");
        apiResponse = getAPIResponse(userQuery); // Get response from the API
    } else {
        apiResponse = "No query provided.";
    }
    server.sendHeader("Location", "/"); // Redirect to the root page
    server.send(303); // HTTP status code for redirection
}

// Function to fetch response from the API
String getAPIResponse(String query) {
    if (WiFi.status() != WL_CONNECTED) {
        return "WiFi not connected.";
    }

    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"contents\":[{\"parts\":[{\"text\":\"" + query +" is the animal just tell 4 things 1 average heart rate,2 average body temperature,3 amount of hours the animal sleeps,4 few tips to take care of the animal. Use emojis for numbers and some part of content, include animal name with emoji at the start, dont use bold."+"\"}]}]}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
        String response = http.getString();
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, response);

        if (!error) {
            const char* text = doc["candidates"][0]["content"]["parts"][0]["text"];
            http.end();
            return String(text);
        } else {
            http.end();
            return "Error parsing JSON: " + String(error.c_str());
        }
    } else {
        String errorMsg = "HTTP error: " + String(http.errorToString(httpResponseCode).c_str());
        http.end();
        return errorMsg;
    }
}

// Function to handle MPU6050 data request
void handleMPUData() {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    float totalAcceleration = sqrt(pow(accel.acceleration.x, 2) +
                                    pow(accel.acceleration.y, 2) +
                                    pow(accel.acceleration.z, 2));

    unsigned long currentTime = millis();

    // Check if acceleration exceeds the threshold
    if (totalAcceleration > activityThresholdMax || totalAcceleration < activityThresholdMin) {
        lastActiveTime = currentTime;
        isResting = false;
    } else if (currentTime - lastActiveTime > inactivityTimeLimit) {
        isResting = true;
    }

    // Update activity data for chart
    if (currentTime - lastChartUpdate >= chartUpdateInterval) {
        activityData += isResting ? "0," : "1,";
        lastChartUpdate = currentTime;
    }

    String activityStatus = isResting ? "Resting" : "Active";

    String jsonResponse = "{\"x\":" + String(accel.acceleration.x, 2) +
                          ",\"y\":" + String(accel.acceleration.y, 2) +
                          ",\"z\":" + String(accel.acceleration.z, 2) +
                          ",\"status\":\"" + activityStatus + "\"}";
    server.send(200, "application/json", jsonResponse);
}

// Function to handle GPS data
void handleGPSData() {
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        gps.encode(c);

        if (gps.location.isUpdated()) {
            latitude = gps.location.lat();
            longitude = gps.location.lng();
        }
    }

    String jsonResponse = "{\"latitude\":" + String(latitude, 6) +
                          ",\"longitude\":" + String(longitude, 6) + "}";
    server.send(200, "application/json", jsonResponse);
}

// Function to handle chart data
void handleChartData() {
    String jsonResponse = "{\"activity\":[" + activityData.substring(0, activityData.length() - 1) + "]}";
    server.send(200, "application/json", jsonResponse);
    activityData = ""; // Clear after sending
}

void setup() {
    Serial.begin(115200);
    delay(10);

    pinMode(redLedPin, OUTPUT);
    pinMode(blueLedPin, OUTPUT);
    pinMode(HEART_RATE_SENSOR_PIN, INPUT);  // Initialize heart rate sensor pin

    digitalWrite(redLedPin, HIGH);
    digitalWrite(blueLedPin, LOW);
    gpsSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("GPS Module Initialized.");

    if (!mpu.begin()) {
        Serial.println("MPU6050 initialization failed!");
        while (1);
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/submit", handleFormSubmit);
    server.on("/mpu", handleMPUData);
    server.on("/gps", handleGPSData);
    server.on("/chart", handleChartData);

    server.begin();
    Serial.println("HTTP server started");
    }

void loop() {
    server.handleClient();

    static bool lastState = LOW;  // Track the previous state of the sensor
    bool currentState = digitalRead(HEART_RATE_SENSOR_PIN);  // Read the sensor state

    // Detect a rising edge (heartbeat detected)
    if (currentState == HIGH && lastState == LOW) {
        currentBeatTime = millis();  // Record the current time
        unsigned long timeDifference = currentBeatTime - previousBeatTime; // Time between beats

        if (timeDifference > 300) {  // Ignore noise (minimum time between beats is ~300ms for ~200 BPM)
            bpm = 60000 / timeDifference;  // Calculate BPM
            beatCount++;
            Serial.print("Beat detected! Current BPM: ");
            Serial.println(bpm);
        }

        previousBeatTime = currentBeatTime;  // Update the last beat time
    }

    lastState = currentState;  // Update the last state

    // Calculate average BPM every 10 seconds
    if (millis() - lastCalculationTime >= bpmCalculationInterval) {
        Serial.print("Average BPM over the last 10 seconds: ");
        Serial.println(beatCount * 6);  // Multiply beats by 6 to get BPM for 1 minute
        beatCount = 0;  // Reset the beat count
        lastCalculationTime = millis();  // Update the last calculation time
    }
}