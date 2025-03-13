#include <WiFi.h>
#include <WebSocketsServer.h>
#include "time.h"
#include <WebServer.h> // Include WebServer library

#define trigPin 9
#define echoPin 10

const char* ssid = "1234";      // Change to your WiFi SSID
const char* password = "hayday123";  // Change to your WiFi Password

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket server on port 81
WebServer server(80); // HTTP server on port 80

int counter = 0;
int currentState = 0;
int previousState = 0;

const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Laxonar</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        .inner-front-page { position: absolute; top: 500px; left: 40%; margin-top: 76px; padding-inline: 16px; }
        .live-number-title { color: white; font-size: 50px; font-weight: bold; }
        #sensorData { color: white; font-size: 30px; }
        .backdrop { position:relative; width: 100%; height: 100vh; background-size: cover; background-position: center; }
        .blinking-dot { --blinking-dot-size: 50px; --blinking-dot-speed: 2s; position: absolute; top: 12%; left: 100%; width: var(--blinking-dot-size); height: var(--blinking-dot-size); background-color: red; border-radius: 50%; animation: blink-animation var(--blinking-dot-speed) infinite alternate; }
        @keyframes blink-animation { 0% { opacity: 1; } 100% { opacity: 0; } }
    </style>
</head>
<body>
    <section>
        <div class="backdrop" style="background-image: url(https://stiimaquacluster.no/wp-content/uploads/2023/03/Nautilus_komplett_illustrasjon_1_JR_2023_v2_16-9.png);" role="img"></div>
        <div class="inner-front-page">
            <div class="live-number-title">Live sensor data: <span class="blinking-dot">&nbsp;</span></div>
            <p id="sensorData">Waiting for data...</p>
        </div>
    </section>
    <script>
        let socket = new WebSocket("ws://172.20.10.9:81/");
        socket.onmessage = function(event) {
            document.getElementById("sensorData").innerText = "Sensor Value: " + event.data;
        };
    </script>
</body>
</html>
)rawliteral";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.println("Client connected");
            break;
        case WStype_DISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case WStype_TEXT:
            Serial.printf("Received: %s\n", payload);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi!");

    // Configure NTP time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Start WebSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("WebSocket server started!");

    // Start HTTP server to serve the HTML page
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", htmlContent);
    });
    server.begin();

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); // Prints the IP address
}

void loop() {
    webSocket.loop();  // Keep WebSocket running
    server.handleClient();  // Handle HTTP requests

    long duration, distance;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = (duration / 2) / 29.1;

    if (distance <= 10) {
        currentState = 1;
    } else {
        currentState = 0;
    }

    delay(100);
    if (currentState != previousState) {
        if (currentState == 1) {
            counter++;

            // Get and print the current time
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                char timeString[30]; // Buffer to hold formatted time
                strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
                Serial.printf("Counter: %d, Time: %s\n", counter, timeString);

                // Send counter + timestamp to WebSocket clients
                String message = String(counter) + " | " + String(timeString);
                webSocket.broadcastTXT(message);
            } else {
                Serial.println("Failed to get time.");
            }
        }
    }
    previousState = currentState;
}