window.onscroll = function() {myFunction()};

var header = document.getElementById("fixedHeader");
var sticky = header.offsetTop;

function myFunction() {
    if (window.scrollY > sticky) {
    header.classList.add("sticky");
    } else {
    header.classList.remove("sticky");
    }
}

let socket = new WebSocket("ws://172.20.10.10:81/");
socket.onmessage = function(event) {
const data = event.data;

// Handles different types of message types
if (data.startsWith("HISTORY,")) {
    const parts = data.split(",");
    if (parts.length >= 4) {
    const counter = parts[1].trim();
    const time = parts[2].trim();
    const date = parts[3].trim();
    addHistoryEntry(counter, time, date);
    }
} else if (data === "CLEAR_HISTORY") {
    document.getElementById("fishHistoryBody").innerHTML = "";
} else if (data.startsWith("WEATHER_")) {
    // New weather data handling
    const parts = data.split(",");
    if (parts.length >= 2) {
        const weatherType = parts[0];
        const weatherValue = parts[1];
        updateWeatherDisplay(weatherType, weatherValue);
    }
} else {
    document.getElementById("sensorData").innerText = "Fish No.: " + event.data;
}
    // When we get a new detection, request the updated history
    if (data.startsWith("HISTORY,") || !data.startsWith("WEATHER_")) {
        requestHistoryUpdate();
    }
};

function updateWeatherDisplay(type, value) {
  const columns = document.querySelectorAll(".weatherColumn");
  
  switch (type) {
      case "WEATHER_MAIN":
          columns[0].querySelectorAll(".weatherRow")[0].querySelector(".value").textContent = value;
          break;
      case "WEATHER_DESC":
          columns[0].querySelectorAll(".weatherRow")[1].querySelector(".value").textContent = value;
          break;
      case "WEATHER_TEMP":
          columns[0].querySelectorAll(".weatherRow")[2].querySelector(".value").textContent = value;
          break;
      case "WEATHER_FEELS":
          columns[0].querySelectorAll(".weatherRow")[3].querySelector(".value").textContent = value;
          break;
      case "WEATHER_HUMIDITY":
          columns[0].querySelectorAll(".weatherRow")[4].querySelector(".value").textContent = value;
          break;
      case "WEATHER_SEA_LEVEL":
          columns[0].querySelectorAll(".weatherRow")[5].querySelector(".value").textContent = value;
          break;
      case "WEATHER_GRND_LEVEL":
          columns[0].querySelectorAll(".weatherRow")[6].querySelector(".value").textContent = value;
          break;

      case "WEATHER_WIND_SPEED":
          columns[1].querySelectorAll(".weatherRow")[0].querySelector(".value").textContent = value;
          break;
      case "WEATHER_WIND_DEG":
          columns[1].querySelectorAll(".weatherRow")[1].querySelector(".value").textContent = value;
          break;
      case "WEATHER_WIND_GUST":
          columns[1].querySelectorAll(".weatherRow")[2].querySelector(".value").textContent = value;
          break;
      case "WEATHER_CLOUDS":
          columns[1].querySelectorAll(".weatherRow")[3].querySelector(".value").textContent = value;
          break;
      case "WEATHER_VISIBILITY":
          columns[1].querySelectorAll(".weatherRow")[4].querySelector(".value").textContent = value;
          break;

      case "WEATHER_TIME":
          columns[2].querySelectorAll(".weatherRow")[0].querySelector(".value").textContent = value;
          break;
      case "WEATHER_CITY":
          columns[2].querySelectorAll(".weatherRow")[1].querySelector(".value").textContent = value;
          break;
      case "WEATHER_COUNTRY":
          columns[2].querySelectorAll(".weatherRow")[2].querySelector(".value").textContent = value;
          break;

      case "WEATHER_STATUS":
          // document.querySelector(".weatherContainer .status").textContent = "Last update: " + value;
          const now = new Date();
          const hours = now.getHours().toString().padStart(2, '0');
          const minutes = now.getMinutes().toString().padStart(2, '0');
          const seconds = now.getSeconds().toString().padStart(2, '0');
          const timeString = `${hours}:${minutes}:${seconds}`;
          
          document.querySelector(".weatherContainer .status").textContent = 
              `Last update: ${timeString} (Weather data updated successfully)`;
          break;
  }
}

// Function to request weather data from ESP32
function requestWeatherUpdate() {
    socket.send("REQUEST_WEATHER");
}

// This function requests history from ESP32
function requestHistoryUpdate() {
socket.send("REQUEST_HISTORY");
} 

// Function to add a new entry to the history table
function addHistoryEntry(counter, time, date) {
const tableBody = document.getElementById("fishHistoryBody");
const newRow = document.createElement("tr");

const counterCell = document.createElement("td");
counterCell.textContent = counter;

const timeCell = document.createElement("td");
timeCell.textContent = time;

const dateCell = document.createElement("td");
dateCell.textContent = date;

newRow.appendChild(counterCell);
newRow.appendChild(timeCell);
newRow.appendChild(dateCell);

// Insert at the beginning for newest entries at the top
tableBody.insertBefore(newRow, tableBody.firstChild);
}

// Inital history request when page loads.
window.onload = function() {
    setTimeout(function() {
        requestHistoryUpdate();
        requestWeatherUpdate();
    }, 10000); // Slight delay to ensure connection is established
    
    // Auto refresh weather data every minute
    setInterval(requestWeatherUpdate, 60000);
}

document.querySelectorAll('.nav-link').forEach(link => {
  link.addEventListener('click', () => {
      document.getElementById('page-nav-toggle').checked = false;
  });
});

// // Update timer and data
// function updateTimer() {
//   fetch('/timer') 
//     .then(response => response.json())
//     .then(data => {
//       document.getElementById('timer').innerText = data.remainingTime;
//       document.getElementById('timeOfDay').innerText = data.timeOfDay;
      
//       // Update period label
//       let period = data.currentPeriod;
//       let periodText = '';
//       switch(period) {
//         case 0: periodText = "00:00-05:59"; break;
//         case 1: periodText = "06:00-11:59"; break;
//         case 2: periodText = "12:00-17:59"; break;
//         case 3: periodText = "18:00-23:59"; break;
//       }
//       document.getElementById('periodLabel').innerText = "Current period: " + periodText;
//     });
// }

// Mock timer data - represents what would come from the server
let mockTimerData = {
  remainingTime: "Timer 17:38",
  currentPeriod: 1,
  timeOfDay: "Time of day: 09:15"
};

function updateTimer() {
  // Use local mock data instead of fetching from server
  document.getElementById('timer').innerText = mockTimerData.remainingTime;
  document.getElementById('timeOfDay').innerText = mockTimerData.timeOfDay;
  
  // Update period label
  let period = mockTimerData.currentPeriod;
  let periodText = '';
  switch(period) {
    case 0: periodText = "00:00-05:59"; break;
    case 1: periodText = "06:00-11:59"; break;
    case 2: periodText = "12:00-17:59"; break;
    case 3: periodText = "18:00-23:59"; break;
  }
  document.getElementById('periodLabel').innerText = "Current period: " + periodText;
}

// Simulate timer countdown - updates the mock data
function simulateTimerCountdown() {
  // Parse current time
  let [minutes, seconds] = mockTimerData.remainingTime.split(':').map(Number);
  
  // Decrease by 1 second
  if (seconds > 0) {
    seconds--;
  } else if (minutes > 0) {
    minutes--;
    seconds = 59;
  } else {
    // Reset timer when it reaches 0
    minutes = 2;
    seconds = 45;
  }
  
  // Update mock data
  mockTimerData.remainingTime = `${minutes}:${seconds < 10 ? '0' : ''}${seconds}`;
  
  // Update display
  updateTimer();
}

// function updateData() {
//   fetch('/data')
//     .then(response => response.json())
//     .then(data => {
//       const maxValue = Math.max(...data.counters, 10);
      
//       for (let i = 0; i < 4; i++) {
//         const heightPercent = (data.counters[i] / maxValue) * 100;
//         document.getElementById('bar' + i).style.height = heightPercent + '%';
//       }
//     });
// }

// Sample data - you would update this manually or through other means
let mockCountersData = {
  counters: [5, 8, 10, 7]
};

function updateData() {
  const data = mockCountersData;
  const maxValue = Math.max(...data.counters, 10);
  
  for (let i = 0; i < 4; i++) {
    const heightPercent = (data.counters[i] / maxValue) * 100;
    document.getElementById('bar' + i).style.height = heightPercent + '%';
  }
}

// For testing, you could update the mock data periodically
function simulateDataChanges() {
  // Generate some random values
  mockCountersData.counters = [
    Math.floor(Math.random() * 10) + 1,
    Math.floor(Math.random() * 10) + 1,
    Math.floor(Math.random() * 10) + 1,
    Math.floor(Math.random() * 10) + 1
  ];
  
  // Update the display
  updateData();
}

// Update every second
setInterval(updateTimer, 1000);
setInterval(updateData, 5000);

// Initial updates
updateTimer();
updateData();