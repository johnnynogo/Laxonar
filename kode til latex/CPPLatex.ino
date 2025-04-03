#include <WiFi.h>
#include <WebSocketsServer.h>
#include "time.h"
#include <WebServer.h> // Include WebServer library
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <SPIFFS.h>


#define trigPin 17
#define echoPin 16

const char* ssid = "1234";      // Change to your WiFi SSID
const char* password = "hayday123";  // Change to your WiFi Password
#define API_KEY "AIzaSyBT6acRzfEP8qXoRaP5F78WjD7fuMo0bTA"
#define DATABASE_URL "https://esp32-laxonar-test-default-rtdb.europe-west1.firebasedatabase.app/"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket server on port 81
WebServer server(80); // HTTP server on port 80

int counter = 0;
int currentState = 0;
int previousState = 0;

// Three Firebase objects
FirebaseData fbdo; // Handles data when there is a change on a database node
FirebaseAuth auth; // Needed for authentication
FirebaseConfig config; // Needed for config

char timeString[30]; // Buffer to hold formatted time
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
String dateTime = "";

const int MAX_HISTORY_ITEMS = 200; // The limit of numbers to retrieve

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

void sendHistoryToClient(uint8_t clientNum) {
    if (Firebase.ready() && signupOK) {
        Serial.println("Fetching history data from Firebase...");

        if (Firebase.RTDB.getJSON(&fbdo, "Sensor")) {
            if (fbdo.dataType() == "json") {
                FirebaseJson &json = fbdo.jsonObject();
                size_t len = json.iteratorBegin();
                FirebaseJson::IteratorValue value;
                int count = 0;

                webSocket.sendTXT(clientNum, "CLEAR_HISTORY"); // Sends a message to clear the current history table

                // Function that processes each entry in Firebase JSON data
                for (size_t i = 0; i < len && count < MAX_HISTORY_ITEMS; i++) {
                    value = json.valueAt(i);

                    if (value.type == FirebaseJson::JSON_OBJECT) {
                        FirebaseJson entryJson;
                        FirebaseJsonData counterData;
                        FirebaseJsonData timeData;

                        entryJson.setJsonData(value.value); // This will parse the entry into a new JSON object

                        // Want to extract the counter and time values
                        entryJson.get(counterData, "counter");
                        entryJson.get(timeData, "time");
                        
                        if(counterData.success && timeData.success) {
                            // Format the message to: HISTORY,counter,timestamp
                            String historyMsg = "HISTORY, " + counterData.stringValue + ", " + timeData.stringValue;
                            webSocket.sendTXT(clientNum, historyMsg);
                            count++;
                        }
                    }
                }

                json.iteratorEnd();
                Serial.printf("Sent %d history items to client\n", count);

            }
        } else {
            Serial.println("Failed to get history data from Firebase" + fbdo.errorReason());
        }
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

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); // Prints the IP address

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    // Note that "", "" is because we have anonymous authentication in Firebase settings
    if(Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Signup OK");
        signupOK = true;
    } else {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // Initialize SPIFFS for font
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // Serve the font file
    server.on("/ZilapMarine.ttf", HTTP_GET, []() {
        File fontFile = SPIFFS.open("/ZilapMarine.ttf", "r");
        if(fontFile){
            server.streamFile(fontFile, "font/ttf");
            fontFile.close();
        } else {
            server.send(404, "text/plain", "Font file not found");
        }
    });

    server.begin();
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
                strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
                Serial.printf("Counter: %d, Time: %s\n", counter, timeString);

                // Send counter + timestamp to WebSocket clients
                String message = String(counter) + " | " + String(timeString);
                webSocket.broadcastTXT(message);

                // FIREBASE CODE
                if (Firebase.ready() && signupOK) {
                    // sendDataPrevMillis = millis(); // No longer needed as we dont want update every 5 seconds
                
                    String counterStr = String(counter);
                    String dateTimeStr = String(timeString); // Convert timeString to String

                    // Creates a unique key for each data entry
                    String dataHolderKey = String(millis()); // Using millis() for a more robust unique key

                    FirebaseJson jsonData;
                    jsonData.set("counter", counterStr);
                    jsonData.set("time", dateTimeStr);
                    String jsonString;

                    // Setting the data in the database with a unique key
                    if (Firebase.RTDB.setJSON(&fbdo, "Sensor/" + dataHolderKey, &jsonData)) {
                        Serial.println("");
                        jsonData.toString(jsonString, false); // Converting JSON object to string
                        Serial.print(jsonString);
                        Serial.print(" - successfully saved to: Sensor/" + dataHolderKey);
                        Serial.println(" (" + fbdo.dataType() + ")");
                    } else {
                        Serial.println("Failed to save data to database. FAILED: " + fbdo.errorReason());
                    }
                }
            } else {
                Serial.println("Failed to get time.");
            }

            for (uint8_t i = 0; i < webSocket.connectedClients(); i++) {
                sendHistoryToClient(i);
            }
        }
        previousState = currentState;
    }
}


// FUNCTIONS: set, setInt, setFloat, setDouble, setString, setJSON, setArray, setBlob, setFile