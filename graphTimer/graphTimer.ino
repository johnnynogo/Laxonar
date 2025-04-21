#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "1234";
const char* password = "hayday123";

WebServer server(80);

// Timer variables
unsigned long timerDuration = 10 * 60 * 1000; // 10 minutes in milliseconds
unsigned long periodDuration = 2.5 * 60 * 1000; // 2.5 minutes in milliseconds
unsigned long myTimerStart = 0; // Changed name to avoid conflict with ESP32 core function
int counters[4] = {0, 0, 0, 0}; // Counters for each time period

// Counter to increment (simulating data collection)
unsigned long lastCountIncrement = 0;
const unsigned long countIncrementInterval = 10000; // Every 10 seconds

// Function declarations to prevent any potential compilation issues
void handleRoot();
void handleTimer();
void handleData();
int getCurrentPeriod();
String getCurrentTimeOfDay();

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/timer", handleTimer);
  server.on("/data", handleData);
  
  server.begin();
  Serial.println("HTTP server started");
  
  // Initialize timer
  myTimerStart = millis(); // Changed variable name
  Serial.println("Timer started");
}

void loop() {
  server.handleClient();
  
  // Simulate data collection for the current time period
  if (millis() - lastCountIncrement >= countIncrementInterval) {
    lastCountIncrement = millis();
    
    // Get current period (0-3)
    int currentPeriod = getCurrentPeriod();
    counters[currentPeriod]++;
    
    Serial.print("Incremented counter for period ");
    Serial.print(currentPeriod);
    Serial.print(" to ");
    Serial.println(counters[currentPeriod]);
  }
  
  // Check if timer needs to restart
  unsigned long elapsed = millis() - myTimerStart; // Changed variable name
  if (elapsed >= timerDuration) {
    myTimerStart = millis(); // Changed variable name
    Serial.println("Timer restarted");
  }
}

int getCurrentPeriod() {
  unsigned long elapsed = millis() - myTimerStart; // Changed variable name
  return (elapsed / periodDuration) % 4;
}

String getCurrentTimeOfDay() {
  unsigned long elapsed = millis() - myTimerStart; // Changed variable name
  int period = (elapsed / periodDuration) % 4;
  
  int minutesInPeriod = (elapsed % periodDuration) / 1000 / 60;
  int secondsInPeriod = (elapsed % periodDuration) / 1000 % 60;
  
  int hour = 0;
  int minute = 0;
  
  // Calculate hours and minutes based on period and elapsed time within period
  switch (period) {
    case 0: // 00:00-05:59
      hour = map(minutesInPeriod * 60 + secondsInPeriod, 0, 150, 0, 6);
      minute = map((minutesInPeriod * 60 + secondsInPeriod) % 25, 0, 25, 0, 60); // Fixed calculation
      break;
    case 1: // 06:00-11:59
      hour = map(minutesInPeriod * 60 + secondsInPeriod, 0, 150, 6, 12);
      minute = map((minutesInPeriod * 60 + secondsInPeriod) % 25, 0, 25, 0, 60); // Fixed calculation
      break;
    case 2: // 12:00-17:59
      hour = map(minutesInPeriod * 60 + secondsInPeriod, 0, 150, 12, 18);
      minute = map((minutesInPeriod * 60 + secondsInPeriod) % 25, 0, 25, 0, 60); // Fixed calculation
      break;
    case 3: // 18:00-23:59
      hour = map(minutesInPeriod * 60 + secondsInPeriod, 0, 150, 18, 24);
      minute = map((minutesInPeriod * 60 + secondsInPeriod) % 25, 0, 25, 0, 60); // Fixed calculation
      break;
  }
  
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", hour, minute);
  return String(timeStr);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Timer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      text-align: center;
      background-color: #f0f0f0;
    }
    
    .container {
      max-width: 800px;
      margin: 0 auto;
      background-color: white;
      border-radius: 8px;
      padding: 20px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    
    h1 {
      color: #333;
    }
    
    .timer {
      font-size: 48px;
      margin: 20px 0;
      font-weight: bold;
      color: #0066cc;
    }
    
    .time-of-day {
      font-size: 24px;
      margin: 10px 0;
      color: #333;
    }
    
    .period-label {
      font-size: 18px;
      margin: 10px 0;
      color: #666;
    }
    
    .chart-container {
      height: 300px;
      margin-top: 40px;
      position: relative;
    }
    
    .chart {
      display: flex;
      height: 250px;
      align-items: flex-end;
      justify-content: space-around;
      padding-bottom: 30px;
    }
    
    .bar {
      width: 60px;
      background-color: #0066cc;
      position: relative;
      transition: height 0.5s ease;
    }
    
    .bar-label {
      position: absolute;
      bottom: -25px;
      width: 100%;
      text-align: center;
      font-size: 14px;
    }
    
    .y-axis {
      position: absolute;
      left: 0;
      height: 250px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      width: 40px;
    }
    
    .y-label {
      text-align: right;
      font-size: 12px;
      padding-right: 5px;
    }
    
    .x-axis {
      position: absolute;
      bottom: 0;
      width: 100%;
      height: 30px;
      display: flex;
      justify-content: space-around;
    }
    
    .x-label {
      text-align: center;
      font-size: 14px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Timer</h1>
    
    <div class="timer" id="timer">00:00</div>
    <div class="time-of-day" id="timeOfDay">00:00</div>
    <div class="period-label" id="periodLabel">Current period: Morning (06:00-11:59)</div>
    
    <div class="chart-container">
      <div class="y-axis">
        <div class="y-label">10</div>
        <div class="y-label">8</div>
        <div class="y-label">6</div>
        <div class="y-label">4</div>
        <div class="y-label">2</div>
        <div class="y-label">0</div>
      </div>
      
      <div class="chart">
        <div class="bar" id="bar0" style="height:0%">
          <div class="bar-label">00:00-05:59</div>
        </div>
        <div class="bar" id="bar1" style="height:0%">
          <div class="bar-label">06:00-11:59</div>
        </div>
        <div class="bar" id="bar2" style="height:0%">
          <div class="bar-label">12:00-17:59</div>
        </div>
        <div class="bar" id="bar3" style="height:0%">
          <div class="bar-label">18:00-23:59</div>
        </div>
      </div>
    </div>
  </div>

  <script>
    // Update timer and data
    function updateTimer() {
      fetch('/timer')
        .then(response => response.json())
        .then(data => {
          document.getElementById('timer').innerText = data.remainingTime;
          document.getElementById('timeOfDay').innerText = data.timeOfDay;
          
          // Update period label
          let period = data.currentPeriod;
          let periodText = '';
          switch(period) {
            case 0: periodText = "Night (00:00-05:59)"; break;
            case 1: periodText = "Morning (06:00-11:59)"; break;
            case 2: periodText = "Afternoon (12:00-17:59)"; break;
            case 3: periodText = "Evening (18:00-23:59)"; break;
          }
          document.getElementById('periodLabel').innerText = "Current period: " + periodText;
        });
    }
    
    function updateData() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          const maxValue = Math.max(...data.counters, 10);
          
          for (let i = 0; i < 4; i++) {
            const heightPercent = (data.counters[i] / maxValue) * 100;
            document.getElementById('bar' + i).style.height = heightPercent + '%';
          }
        });
    }
    
    // Update every second
    setInterval(updateTimer, 1000);
    setInterval(updateData, 5000);
    
    // Initial updates
    updateTimer();
    updateData();
  </script>
</body>
</html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}

void handleTimer() {
  unsigned long elapsed = millis() - myTimerStart; // Changed variable name
  unsigned long remaining = (timerDuration > elapsed) ? (timerDuration - elapsed) : 0;
  
  // If timer completed, restart it
  if (remaining == 0) {
    myTimerStart = millis(); // Changed variable name
    remaining = timerDuration;
  }
  
  int minutes = remaining / 60000;
  int seconds = (remaining % 60000) / 1000;
  
  int currentPeriod = getCurrentPeriod();
  String timeOfDay = getCurrentTimeOfDay();
  
  String json = "{";
  json += "\"remainingTime\":\"" + String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds) + "\",";
  json += "\"currentPeriod\":" + String(currentPeriod) + ",";
  json += "\"timeOfDay\":\"" + timeOfDay + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleData() {
  String json = "{\"counters\":[";
  for (int i = 0; i < 4; i++) {
    json += String(counters[i]);
    if (i < 3) json += ",";
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}