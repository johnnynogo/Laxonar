#include <WiFi.h>
#include <WebSocketsServer.h>
#include "time.h"
#include <WebServer.h> // Include WebServer library
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"


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

const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Laxonar</title>


    <style>
        /* ************************************************* */

body {
    /* background-image: url("https://www.akvagroup.no/getfile.php/135852-1663322966/Bildegalleri/Sea%20Based/Bilder%20og%20sertifikater/Deep%20farming/Atlantis/Luftkuppel_dyp_drift_web.png%20%28optimized_original%29.png"); */
    /*
    background-image: url("https://images.dngroup.com/image/eyJ3IjoxOTYwLCJmIjoid2VicCIsImsiOiJiMDE4YzEwZDJmYmFlYTNjOGFhOTY1OGRiYzk3NmE4NCIsImNyb3AiOlsyMDEsMCwxMTk3LDgwMF0sInIiOjEuNSwibyI6ImRuIn0");
    background-repeat: no-repeat;
    background-size: cover; 
    */
    background-color: rgba(255, 255, 255);
}

* {
  margin: 0;
  padding: 0;
  box-sizing: border-box; /* Prevents padding from affecting width */
}

@font-face {
  font-family: 'zilmar'; /*a name to be used later*/
  src: url(ZilapMarine.ttf); /*URL to font*/
}

/* ************************************************* */
/* HEADER START */

#fixedHeader {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  padding: 30px 30px;
  background: white;
  opacity: 0.9;
  z-index: 1000;
}

.zilap-font {
  font-family: 'zilmar';
  font-size: 50px;
  color: black;
}

/* HEADER END */
/* ************************************************* */


/* ************************************************* */
/* COVER PAGE START */

.backdrop {
  position:relative;
  margin: 0;
  padding: 0;
  top: 0px;
  left: 0px;
  width: 100%;
  height: 100vh; /* Full screen height */
  background-size: cover;
  background-position: center;
}

.inner-front-page {
  position: absolute;
  top: 500px;
  left: 40%;
  margin-top: 76px;
  padding-block-start: 0;
  padding-inline: 16px;
}

.live-number-title {
  color: white;
  font-size: 50px;
  font-weight: bold;
}

#sensorData {
  color: white;
  font-size: 30px;
}

.blinking-dot {
  --blinking-dot-size: 35px;
  --blinking-dot-speed: 2s;

  position: absolute;
  top: 13%;
  left: 100%;
  display: inline-block;
  width: var(--blinking-dot-size);
  height: var(--blinking-dot-size);
  background-color: red; /* Change color as needed */
  border-radius: 50%; /* Makes it a circle */

  animation: blink-animation var(--blinking-dot-speed) infinite alternate;
}

@keyframes blink-animation {
  0% {
    opacity: 1;
  }
  100% {
    opacity: 0;
  }
}

/* COVER PAGE END */
/* ************************************************* */

/* ************************************************* */
/* MAIN PAGE START */

.content {
    width: 100%;
    padding: 14px;
    color: black;
    background-color: rgba(243, 241, 239, 0.4); /* setting transparecy only on background color */
    /* not opacity: 0.6; as opacity is applied to the entire element and all of its children*/
  }

.boxOne {
  /* background-color: rgba(243, 241, 239, 1); */
  background-color: red;
  width: 50%;
  height: 50%;
  float: left;
}

.boxTwo {
  /* background-color: rgba(253, 241, 239, 1); */
  background-color: green;
  width: 50%;
  height: 50%;
  float: right;
}

.boxThree {
  /* background-color: rgba(263, 241, 239, 1); */
  background-color: blue;
  width: 50%;
  height: 50%;
  float: left;
}

.boxFour {
  /* background-color: rgba(273, 241, 239, 1); */
  background-color: yellow;
  width: 50%;
  height: 50%;
  float: right;
}

/* FISH HISTORY SECTION START */
.fish-history-container {
  margin-top: 20px;
  max-height: 300px;
  overflow-y: auto;
}

#fishHistoryTable {
  width: 100%;
  border-collapse: collapse;
  margin-bottom: 20px;
}

#fishHistoryTable th, #fishHistoryTable td {
  border: 1px solid black;
  padding: 8px;
  text-align: left;
}

#fishHistoryTable tr:nth-child(even) {
  background-color: #f2f2f2;
}

#fishHistoryTable th {
  padding-top: 12px;
  padding-bottom: 12px;
  background-color: #4CAF50;
  color: white;
}
/* FISH HISTORY SECTION END */

/* MAIN PAGE END */
/* ************************************************* */
    </style>
</head>


<body>
    <header id="fixedHeader">
        <h2 class="zilap-font">Laxonar</h2>
    </header> 


    <!-- Section 1: Cover Image -->
    <section>
        <div class="backdrop" style="background-image: url(https://stiimaquacluster.no/wp-content/uploads/2023/03/Nautilus_komplett_illustrasjon_1_JR_2023_v2_16-9.png);" role="img"></div>
        <div class="inner-front-page">
            <div class="live-number-title">Live sensor data: <span class="blinking-dot">&nbsp;</span></div>
            <br>
            <p id="sensorData">Waiting for data...</p>
        </div>
    </section>

    <!-- Section 2: Main Content -->
        <main class="content">
            <div class="boxOne">En</div>    
            <div class="boxTwo">To</div>    
            <div class="boxThree">Tre</div>    
            <div class="boxFour">Fire</div>

            
            <h1 class="contentTextHeader">Grown Man Shit - Tøyen Holding</h1>
            <br>
            <h2 class="contentText">
                [Vers 1: Mest Seff] [Linje 15-18]<br>

                <!-- Ja, hør her, ja <br>
                Det finnes tre nivåer, det er min holdning <br>
                Dårlig, bra, og Tøyen Holding <br>
                Har 10k bars i min tekstfil <br>
                Zebb som et trekkspill <br>
                Kvinner hyler på en spiller som en skrekkfilm <br>
                Jeg får alltid godkjent når jeg tapper <br>
                Hun sa det kiler i tissen når jeg rapper <br>‹
                Hvert år får vi dyrere uvaner <br>‹
                Du ser røyken på avstand som en vulkaner når jeg fyrer en cubaner <br>
                Skriver kun bars i flowtanks <br>‹
                Le‹gge dе på låta di? No thanks <br>
                Tom Hanks, greiene dеr har mindre edge enn en steambun <br>
                Eneste du har fiksa i livet er PornHub Premium <br> -->

                Please son, hva får deg til å tro du får bli med på skiva? <br>
                Dama di Anita ser ut som Wiz Khalifa <br>
                Litt for mye reefa, energidrikk og FIFA <br>
                Det e'kke min business, men ville bare si fra <br>

                <!-- <br>
                [Vers 2: Fredfades] <br>
                Fredfades, norsk raps svar på Öde Spildo <br>
                Beatsa får chicksa til å riste som en dildo <br>
                Masse folk på pikken, ser ut som monolitten <br>
                Treogtredve, oppe og dunker som Scottie Pippen <br>
                Stilen er såpeglatt, har begynt å gå i frakk <br>
                Blitt rå i sjakk, drikker Fourrier Clos Saint-Jacques <br>
                På yarden med en magnum, kaller det paint n sip <br>
                Mens du er på Paint'n Sip med en basic bitch <br>
                Toxic stemning når vi bro-diner <br>
                På hytta, bøyer hunkjønn som en norsklærer <br>
                Min vinkjeller voktes av en rottweiler <br>
                Mine hoes wilder fordi jeg sponser de med no' Holzweiler <br>
                Bruker kun skimask når vi skal lage bane <br>
                Eneste bønnene du pusher er edamame <br>
                Gir ut fem album før du rekker å lage to sanger <br>
                Grown man shit, du må trekke ned to ganger <br> -->
            </h2>
        </main>

        <!-- Section 3: Fish Detection History -->
         <section class="content">
            <h2>Fish Detection History</h2>
            <div class="fish-history-container">
                <table id="fishHistoryTable">
                    <thead>
                        <tr>
                            <th>Detection #</th>
                            <th>Time</th>
                        </tr>
                    </thead>

                    <tbody id="fishHistoryBody"> <!-- Data will automatically be updated in here --></tbody>
                </table>
            </div>
         </section>


    <script>
        let socket = new WebSocket("ws://172.20.10.9:81/");
        socket.onmessage = function(event) {
          const data = event.data;

          // Handles different types of message types
          if (data.startsWith("HISTORY,")) {
            const parts = data.split(",");
            if (parts.length >= 3) {
              const counter = parts[1];
              const timestamp = parts.slice(2).join(',');
              addHistoryEntry(counter, timestamp);
            }
          } else if (data === "CLEAR_HISTORY") {
            document.getElementById("fishHistoryBody").innerHTML = "";
          } else {
            document.getElementById("sensorData").innerText = "Sensor Value: " + event.data;
          }
            // When we get a new detection, request the updated history
            requestHistoryUpdate();
        };

        // This function requests history from ESP32
        function requestHistoryUpdate() {
        socket.send("REQUEST_HISTORY");
        } 

        // Function to add a new entry to the history table
        function addHistoryEntry(counter, timestamp) {
        const tableBody = document.getElementById("fishHistoryBody");
        const newRow = document.createElement("tr");

        const counterCell = document.createElement("td");
        counterCell.textContent = counter;

        const timeCell = document.createElement("td");
        timeCell.textContent = timestamp;

        newRow.appendChild(counterCell);
        newRow.appendChild(timeCell);

        // Insert at the beginning for newest entries at the top
        tableBody.insertBefore(newRow, tableBody.firstChild);
        }

        // Inital history request when page loads.
        window.onload = function() {
        setTimeout(requestHistoryUpdate, 1000); // Slight delay to ensure connection is established
        }
    </script>


</body>

</html>
)rawliteral";

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
    server.begin();

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