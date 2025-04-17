#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>

// WiFi Credentials
const char* ssid = "1234";
const char* password = "hayday123";

// API Info
String URL = "http://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "66c2342087c98b327aefd3764035bcb4";
String lat = "63.710109052324256";
String lon = "8.560403277009401";

// Weather data variables
DynamicJsonDocument doc(2048);
String weatherData = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long updateInterval = 30000; // 30 seconds

// Create web server object
WebServer server(80);

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Define server routes
  server.on("/", handleRoot);
  server.on("/refresh", handleRefresh);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  
  // Get initial weather data
  getWeatherData();
}

void loop() {
  server.handleClient();
  
  // Update weather data every 30 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - lastWeatherUpdate >= updateInterval) {
    getWeatherData();
    lastWeatherUpdate = currentMillis;
  }
}

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Set HTTP Request Final URL with Location and API key information
    http.begin(URL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey);
    
    // Send HTTP GET request
    int httpCode = http.GET();
    
    // httpCode will be negative on error
    if (httpCode > 0) {
      // Read Data as a JSON string
      String JSON_Data = http.getString();
      Serial.println(JSON_Data);
      
      // Parse JSON
      deserializeJson(doc, JSON_Data);
      
      // Store successful data retrieval
      weatherData = "Weather data updated successfully";
    } else {
      Serial.println("Error getting weather data");
      weatherData = "Error fetching weather data";
    }
    
    http.end();
  } else {
    weatherData = "WiFi not connected";
  }
}

String getTimeString(long timestamp, long timezone) {
  // Convert UTC timestamp to local time considering timezone offset
  unsigned long localTime = timestamp + timezone;
  
  // Calculate hours, minutes, seconds
  int hours = (localTime / 3600) % 24;
  int minutes = (localTime / 60) % 60;
  int seconds = localTime % 60;
  
  // Format time as HH:MM:SS
  String timeStr = "";
  if (hours < 10) timeStr += "0";
  timeStr += String(hours) + ":";
  if (minutes < 10) timeStr += "0";
  timeStr += String(minutes) + ":";
  if (seconds < 10) timeStr += "0";
  timeStr += String(seconds);
  
  return timeStr;
}

void handleRoot() {
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<meta charset='UTF-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "<title>ESP32 Complete Weather Dashboard</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f7fa; }"
                ".container { max-width: 1000px; margin: 0 auto; background-color: white; border-radius: 10px; padding: 20px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
                "h1, h2 { color: #0066cc; text-align: center; }"
                ".section { margin: 20px 0; padding: 15px; border-radius: 8px; background-color: #e6f2ff; }"
                ".data-row { display: flex; justify-content: space-between; margin: 8px 0; padding: 5px 0; border-bottom: 1px solid #ddd; }"
                ".label { font-weight: bold; color: #444; }"
                ".value { color: #0066cc; }"
                ".status { margin-top: 20px; font-style: italic; color: #666; text-align: center; }"
                "button { display: block; margin: 20px auto; background-color: #0066cc; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; font-size: 1em; }"
                "button:hover { background-color: #0055aa; }"
                ".weather-icon { display: block; margin: 10px auto; width: 80px; height: 80px; }"
                "</style>"
                "</head>"
                "<body>"
                "<div class='container'>"
                "<h1>ESP32 Complete Weather Dashboard</h1>";

  // MAIN WEATHER SECTION
  if (doc.containsKey("weather") && doc["weather"].size() > 0) {
    html += "<div class='section'>"
            "<h2>Current Weather</h2>";
    
    // Weather icon
    if (doc["weather"][0].containsKey("icon")) {
      String iconCode = doc["weather"][0]["icon"].as<String>();
      html += "<img class='weather-icon' src='http://openweathermap.org/img/wn/" + iconCode + "@2x.png' alt='Weather Icon'>";
    }
    
    if (doc["weather"][0].containsKey("main")) {
      html += "<div class='data-row'><span class='label'>Main:</span> <span class='value'>" + doc["weather"][0]["main"].as<String>() + "</span></div>";
    }
    
    if (doc["weather"][0].containsKey("description")) {
      html += "<div class='data-row'><span class='label'>Description:</span> <span class='value'>" + doc["weather"][0]["description"].as<String>() + "</span></div>";
    }
    
    if (doc["weather"][0].containsKey("id")) {
      html += "<div class='data-row'><span class='label'>Weather ID:</span> <span class='value'>" + String(doc["weather"][0]["id"].as<int>()) + "</span></div>";
    }
    
    html += "</div>";
  }
  
  // LOCATION SECTION
  html += "<div class='section'>"
          "<h2>Location Information</h2>";
          
  if (doc.containsKey("name")) {
    html += "<div class='data-row'><span class='label'>City:</span> <span class='value'>" + doc["name"].as<String>() + "</span></div>";
  }
  
  if (doc.containsKey("sys") && doc["sys"].containsKey("country")) {
    html += "<div class='data-row'><span class='label'>Country:</span> <span class='value'>" + doc["sys"]["country"].as<String>() + "</span></div>";
  }
  
  if (doc.containsKey("coord")) {
    if (doc["coord"].containsKey("lon")) {
      html += "<div class='data-row'><span class='label'>Longitude:</span> <span class='value'>" + String(doc["coord"]["lon"].as<float>(), 4) + "°</span></div>";
    }
    
    if (doc["coord"].containsKey("lat")) {
      html += "<div class='data-row'><span class='label'>Latitude:</span> <span class='value'>" + String(doc["coord"]["lat"].as<float>(), 4) + "°</span></div>";
    }
  }
  
  if (doc.containsKey("timezone")) {
    int tzHours = doc["timezone"].as<int>() / 3600;
    int tzMins = (abs(doc["timezone"].as<int>()) % 3600) / 60;
    String tzSign = tzHours >= 0 ? "+" : "-";
    html += "<div class='data-row'><span class='label'>Timezone:</span> <span class='value'>UTC" + tzSign + String(abs(tzHours)) + 
            (tzMins > 0 ? ":" + String(tzMins) : "") + "</span></div>";
  }
  
  if (doc.containsKey("id")) {
    html += "<div class='data-row'><span class='label'>City ID:</span> <span class='value'>" + String(doc["id"].as<long>()) + "</span></div>";
  }
  
  html += "</div>";
  
  // TEMPERATURE SECTION
  if (doc.containsKey("main")) {
    html += "<div class='section'>"
            "<h2>Temperature & Pressure</h2>";
    
    if (doc["main"].containsKey("temp")) {
      html += "<div class='data-row'><span class='label'>Temperature:</span> <span class='value'>" + 
              String(doc["main"]["temp"].as<float>(), 2) + " °C</span></div>";
    }
    
    if (doc["main"].containsKey("feels_like")) {
      html += "<div class='data-row'><span class='label'>Feels Like:</span> <span class='value'>" + 
              String(doc["main"]["feels_like"].as<float>(), 2) + " °C</span></div>";
    }
    
    if (doc["main"].containsKey("temp_min")) {
      html += "<div class='data-row'><span class='label'>Min Temperature:</span> <span class='value'>" + 
              String(doc["main"]["temp_min"].as<float>(), 2) + " °C</span></div>";
    }
    
    if (doc["main"].containsKey("temp_max")) {
      html += "<div class='data-row'><span class='label'>Max Temperature:</span> <span class='value'>" + 
              String(doc["main"]["temp_max"].as<float>(), 2) + " °C</span></div>";
    }
    
    if (doc["main"].containsKey("pressure")) {
      html += "<div class='data-row'><span class='label'>Pressure:</span> <span class='value'>" + 
              String(doc["main"]["pressure"].as<int>()) + " hPa</span></div>";
    }
    
    if (doc["main"].containsKey("humidity")) {
      html += "<div class='data-row'><span class='label'>Humidity:</span> <span class='value'>" + 
              String(doc["main"]["humidity"].as<int>()) + "%</span></div>";
    }
    
    if (doc["main"].containsKey("sea_level")) {
      html += "<div class='data-row'><span class='label'>Sea Level Pressure:</span> <span class='value'>" + 
              String(doc["main"]["sea_level"].as<int>()) + " hPa</span></div>";
    }
    
    if (doc["main"].containsKey("grnd_level")) {
      html += "<div class='data-row'><span class='label'>Ground Level Pressure:</span> <span class='value'>" + 
              String(doc["main"]["grnd_level"].as<int>()) + " hPa</span></div>";
    }
    
    html += "</div>";
  }
  
  // WIND SECTION
  if (doc.containsKey("wind")) {
    html += "<div class='section'>"
            "<h2>Wind Information</h2>";
    
    if (doc["wind"].containsKey("speed")) {
      html += "<div class='data-row'><span class='label'>Wind Speed:</span> <span class='value'>" + 
              String(doc["wind"]["speed"].as<float>(), 2) + " m/s</span></div>";
    }
    
    if (doc["wind"].containsKey("deg")) {
      html += "<div class='data-row'><span class='label'>Wind Direction:</span> <span class='value'>" + 
              String(doc["wind"]["deg"].as<int>()) + "°</span></div>";
    }
    
    if (doc["wind"].containsKey("gust")) {
      html += "<div class='data-row'><span class='label'>Wind Gust:</span> <span class='value'>" + 
              String(doc["wind"]["gust"].as<float>(), 2) + " m/s</span></div>";
    }
    
    html += "</div>";
  }
  
  // CLOUDS SECTION
  if (doc.containsKey("clouds") && doc["clouds"].containsKey("all")) {
    html += "<div class='section'>"
            "<h2>Cloud Cover</h2>"
            "<div class='data-row'><span class='label'>Cloudiness:</span> <span class='value'>" + 
            String(doc["clouds"]["all"].as<int>()) + "%</span></div>"
            "</div>";
  }
  
  // VISIBILITY SECTION
  if (doc.containsKey("visibility")) {
    float visibilityKm = doc["visibility"].as<int>() / 1000.0;
    html += "<div class='section'>"
            "<h2>Visibility</h2>"
            "<div class='data-row'><span class='label'>Visibility:</span> <span class='value'>" + 
            String(visibilityKm, 2) + " km</span></div>"
            "</div>";
  }
  
  // SUN TIMES SECTION
  if (doc.containsKey("sys")) {
    html += "<div class='section'>"
            "<h2>Sun Times</h2>";
    
    if (doc["sys"].containsKey("sunrise") && doc.containsKey("timezone")) {
      long sunriseTime = doc["sys"]["sunrise"].as<long>();
      long timezone = doc["timezone"].as<long>();
      String sunriseTimeStr = getTimeString(sunriseTime, timezone);
      html += "<div class='data-row'><span class='label'>Sunrise:</span> <span class='value'>" + sunriseTimeStr + " (local time)</span></div>";
    }
    
    if (doc["sys"].containsKey("sunset") && doc.containsKey("timezone")) {
      long sunsetTime = doc["sys"]["sunset"].as<long>();
      long timezone = doc["timezone"].as<long>();
      String sunsetTimeStr = getTimeString(sunsetTime, timezone);
      html += "<div class='data-row'><span class='label'>Sunset:</span> <span class='value'>" + sunsetTimeStr + " (local time)</span></div>";
    }
    
    html += "</div>";
  }
  
  // DATA INFO SECTION
  html += "<div class='section'>"
          "<h2>Data Information</h2>";
  
  if (doc.containsKey("dt") && doc.containsKey("timezone")) {
    long dataTime = doc["dt"].as<long>();
    long timezone = doc["timezone"].as<long>();
    String dataTimeStr = getTimeString(dataTime, timezone);
    html += "<div class='data-row'><span class='label'>Data Calculation Time:</span> <span class='value'>" + dataTimeStr + " (local time)</span></div>";
  }
  
  if (doc.containsKey("base")) {
    html += "<div class='data-row'><span class='label'>Data Source:</span> <span class='value'>" + doc["base"].as<String>() + "</span></div>";
  }
  
  if (doc.containsKey("cod")) {
    html += "<div class='data-row'><span class='label'>API Response Code:</span> <span class='value'>" + String(doc["cod"].as<int>()) + "</span></div>";
  }
  
  html += "</div>";
  
  // FOOTER
  html += "<p class='status'>Last update: " + weatherData + "</p>"
          "<button onclick='location.href=\"/refresh\"'>Refresh Data</button>"
          "</div>"
          "<script>"
          "setTimeout(function(){ location.reload(); }, 30000);"
          "</script>"
          "</body>"
          "</html>";
  
  server.send(200, "text/html", html);
}

void handleRefresh() {
  getWeatherData();
  server.sendHeader("Location", "/");
  server.send(303);
}