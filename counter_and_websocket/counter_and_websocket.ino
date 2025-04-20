#include <WiFi.h>
#include <WebSocketsServer.h>
#include "time.h"
#include <WebServer.h> // Include WebServer library
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


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


// For implementing weather
// String URL = "https://api.openweathermap.org/data/2.5/weather?lat=63.7101&lon=8.5602&appid=66c2342087c98b327aefd3764035bcb4";
String URL = "http://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "66c2342087c98b327aefd3764035bcb4";
// Credentials for Salmar Froya 
String lon = "8.560392549727782"; // 8.560392549727782 is the full longitude
String lat = "63.710132834995264"; // 63.710132834995264 is the full latitude

DynamicJsonDocument weatherDoc(2048);
String weatherData = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 30000; // 30 seconds


const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html>
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
                background-color: #F0F7EE;
            }

            * {
            margin: 0;
            padding: 0;
            box-sizing: border-box; /* Prevents padding from affecting width */
            }

            @font-face {
            font-family: 'zilmar'; /*a name to be used later*/
            src: url(ZilapMarine.ttf); /*URL to font, note: in .ino there is backslash - \ZilapMarine.ttf */ 
            }

            @font-face {
            font-family: 'helveticaNeue';
            src: url(HelveticaNeue-Roman.otf);
            }

            @font-face {
            font-family: 'NeueHaas';
            src: url(NeueHaasDisplayRoman.ttf);
            }

            @font-face {
            font-family: 'NeueHaasBold';
            src: url(NeueHaasDisplayBold.ttf);
            }

            /* ************************************************* */
            /* MENU BAR START */

            @import url('https://fonts.googleapis.com/css?family=Merriweather:900&display=swap');

            :root {
            --color-primary: rgba(0, 46, 90, 1);
            --color-secondary: #F0F7EE;
            --duration: 1s;
            --nav-duration: calc(var(--duration) / 4);
            --ease: cubic-bezier(0.215, 0.61, 0.355, 1);
            --space: 1rem;
            --font-primary: 'Helvetica', sans-serif;
            --font-heading: 'Merriweather', serif;
            --font-size: 1.125rem;
            --line-height: 1.5;
            }

            .main-navigation-toggle {
            position: fixed;
            height: 1px; 
            width: 1px;
            overflow: hidden;
            clip: rect(1px, 1px, 1px, 1px);
            white-space: nowrap;
            
            + label {
                position: fixed;
                top: calc(var(--space) * 1.5);
                right: calc(var(--space) * 2);
                cursor: pointer;
                z-index: 2000;
            }
            }

            .icon--menu-toggle {
            --size: calc(1rem + 4vmin);
            display: flex;
            align-items: center;
            justify-content: center;
            width: var(--size);
            height: var(--size);
            stroke-width: 6;
            }

            .icon-group {
            transform: translateX(0);
            transition: transform var(--nav-duration) var(--ease);
            }

            .icon--menu {
            stroke: var(--color-primary);
            }

            .icon--close {
            stroke: var(--color-secondary);
            transform: translateX(-100%);
            }

            .main-navigation {
            position: fixed;
            top: 0;
            left: 0;
            display: flex;
            align-items: center;
            width: 100%;
            height: 100%;
            transform: translateX(-100%);
            transition: transform var(--nav-duration);
            z-index: 1;
            
            &:after {
                content: '';
                position: absolute;
                top: 0;
                left: 0;
                width: 100%;
                height: 100%;
                background-color: var(--color-primary);
                transform-origin: 0 50%;
                z-index: -1;
            }
            
            ul {
                font-size: 12vmin;
                font-family: var(--font-heading);
                width: 100%;
            }
            
            li {
                --border-size: 1vmin;
                display: flex;
                align-items: center;
                position: relative;
                overflow: hidden;
                
                &:after {
                content: '';
                position: absolute;
                bottom: 0;
                left: 0;
                width: 100%;
                height: var(--border-size);
                background-color: var(--color-secondary);
                transform-origin: 0 50%;
                transform: translateX(-100%) skew(15deg);
                }
            }
            
            a {
                display: inline-block;
                width: 100%;
                max-width: 800px;
                margin: 0 auto;
                color: var(--color-secondary);
                line-height: 1;
                text-decoration: none;
                user-select: none;
                padding: var(--space) calc(var(--space) * 2) calc(var(--space) + var(--border-size) / 2);
                transform: translateY(100%);
            }
            }

            .main-content {
            margin: 6rem auto;
            max-width: 70ch;
            padding: 0 calc(var(--space) * 2);
            transform: translateX(0);
            transition: transform calc(var(--nav-duration) * 2) var(--ease);
            
            > * + * {
                margin-top: calc(var(--space) * var(--line-height));
            }
            }

            .main-navigation-toggle:checked {
            ~ label .icon--menu-toggle {    
                .icon-group {
                transform: translateX(100%);
                }
            }
            
            ~ .main-content {
                transform: translateX(10%);
            }
            
            ~ .main-navigation {
                transition-duration: 0s;
                transform: translateX(0);
                
                &:after {
                animation: nav-bg var(--nav-duration) var(--ease) forwards;
                }
                
                li:after {
                animation: nav-line var(--duration) var(--ease) forwards;
                }
                
                a {
                animation: link-appear calc(var(--duration) * 1.5) var(--ease) forwards;
                }
                
                @for $i from 1 through 4 {
                li:nth-child(#{$i}) {
                    &:after, a {
                    animation-delay: calc((var(--duration) / 2) * #{$i} * 0.125);
                    }
                }
                }
            }
            }

            @keyframes nav-bg {
            from { transform: translateX(-100%) skewX(-15deg) }
            to { transform: translateX(0) }
            }

            @keyframes nav-line {
            0%   { transform: scaleX(0); transform-origin: 0 50%; }
            35%  { transform: scaleX(1.001); transform-origin: 0 50%; }
            65%  { transform: scaleX(1.001); transform-origin: 100% 50%; }
            100% { transform: scaleX(0); transform-origin: 100% 50%; }
            }

            @keyframes link-appear {
            0%, 25%   { transform: translateY(100%); }
            50%, 100% { transform: translateY(0); }
            }

            /* MENU BAR END */
            /* ************************************************* */

            /* ************************************************* */
            /* HEADER START */

            #fixedHeader {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            padding-left: 20px;
            padding-bottom: 20px;
            background: white;
            opacity: 0.9;
            z-index: 1000;
            /* display: flex; */
            /* align-items: center; Changed from center to allow individual alignment */
            }

            .logo-container {
            display: flex;
            align-items: center; /* vertical center */
            text-decoration: none;
            }

            #logo {
            width: 100px;
            margin-right: 5px;
            padding-top: 15px;

            }

            #fixedHeader a {
            /* display: flex; */
            /* align-items: flex-start; */
            text-decoration: none;
            }

            .zilap-font {
            font-family: 'zilmar';
            position: relative;
            font-size: 50px;
            color: black;
            text-decoration: none;
            padding-top: 20px;
            /* text-align: center; */
            /* align-self: flex-end; */
            }

            a:link, a:visited, a:hover, a:active {
            text-decoration: none;
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
            top: 400px;
            left: 50%;
            transform: translateX(-50%);
            text-align: center;
            width: 60%;
            margin-top: 76px;
            padding-block-start: 0;
            padding-inline: 16px;
            }

            .live-number-title {
            /* text-align: center; */
            color: white;
            font-size: 30px;
            font-family: 'NeueHaas';
            /* font-weight: bold; */
            /* display: flex;
            justify-content: center;
            align-items: center;
            gap: 10px; */
            position: relative;
            display: inline-block;
            }

            #sensorData {
            color: white;
            font-size: 55px;
            min-height: 40px;
            margin: 0;
            font-family: 'NeueHaasBold';
            text-align: center;
            padding: 10px 0px;
            }

            .blinking-dot {
            --blinking-dot-size: 15px;
            --blinking-dot-speed: 1s;

            /* display: inline-block; */
            position: absolute;
            top: 25%;
            right: -25px;
            display: inline-block;
            width: var(--blinking-dot-size);
            height: var(--blinking-dot-size);
            background-color: red; /* Change color as needed */
            border-radius: 50%; /* Makes it a circle */
            /* min-width: calc(var(--blinking-dot-size) * 2); */

            animation: blink-animation var(--blinking-dot-speed) infinite alternate;
            }

            .blinking-dot::before {
            content: '';
            position:absolute;
            top: 0;
            left: 0;
            /* display: inline-block; */
            width: var(--blinking-dot-size);
            height: var(--blinking-dot-size);
            background-color: red; /* Change color as needed */
            border-radius: 50%; /* Makes it a circle */
            }

            .blinking-dot::after {
            content: '';
            position:absolute;
            top: -2px;
            left: -2px;
            display: inline-block;
            width: calc(var(--blinking-dot-size) * 1.25);
            height: calc(var(--blinking-dot-size) * 1.25);
            background-color: red; /* Change color as needed */
            border-radius: 50%; /* Makes it a circle */
            animation: pulse 1000ms infinite;
            }

            @keyframes pulse {
            0% { 
            transform: scale(1); 
            opacity: 1; }
            100% {
                transform: scale(1.5); 
                opacity: 0; }
            }

            @keyframes blink-animation {
            60% {
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
                height: 400px;
                overflow: hidden;
            }

            /* FISH HISTORY SECTION START */
            .fish-history-container {
            margin: 20px 0;
            padding: 15px;
            border-radius: 8px; 
            max-height: 300px;
            overflow-y: auto;
            float: left;
            width: 50%;
            }

            #fishHistoryTable {
            width: 100%;
            border-collapse: collapse;
            margin-bottom: 20px;
            }

            #fishHistoryTable th{
            border: 0.5px solid black;
            padding: 8px;
            text-align: center;
            font-family: "neueHaasBold";
            }

            .fish-detection-history-title {
            font-family: "neueHaasBold";
            color: rgba(0, 46, 90, 1);
            font-size: 30px;
            margin-bottom: 10px;
            text-align: center;
            }

            #fishHistoryTable td {
            border: 0.5px solid black;
            padding: 8px;
            text-align: center;
            font-family: "neueHaas";
            }

            #fishHistoryTable tr:nth-child(even) {
            background-color: #F0F7EE;
            }

            #fishHistoryTable tr:nth-child(odd) { background-color: white; }

            #fishHistoryTable th {
            padding-top: 12px;
            padding-bottom: 12px;
            background-color: rgba(0, 46, 90, 1);
            color: white;
            }

            #fishNumber {
            width: 100px;
            }


            /* FISH HISTORY SECTION END */


            /* GRAPH START */
            .detection-graph-container {
            width: 100%;
            margin: 0 auto;
            border-radius: 10px; 
            padding: 20px; 
            /* box-shadow: 0 2px 10px rgba(0,0,0,0.1); */
            overflow: hidden;
            }

            .graph-container {
            margin: 20px 0;
            padding: 15px;
            border-radius: 8px; 
            max-height: 300px;
            overflow-y: auto;
            float: left;
            width: 50%;
            }

            .graph-title {
            font-family: "neueHaasBold";
            color: rgba(0, 46, 90, 1);
            font-size: 30px;
            margin-bottom: 10px;
            text-align: center;
            }

            /* GRAPH END */


            /* WEATHER DISPLAY START */

            .weatherContainer {
            width: 100%;
            margin: 20px auto 0;
            clear: both;
            background-color: white; 
            border-radius: 10px; 
            padding: 20px; 
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            }

            #weatherH1 {
            font-family: "neueHaasBold";
            color: rgba(0, 46, 90, 1);
            font-size: 30px;
            margin-bottom: 10px;
            text-align: center;
            }

            #weatherH2 {
            font-family: "neueHaasBold";
            color: rgba(0, 46, 90, 1);
            font-size: 20px;
            margin-bottom: 10px;
            text-align: center;
            }

            .weatherColumn {
            margin: 20px 0; 
            padding: 15px; 
            border-radius: 8px; 
            /* background-color: #e6f2ff; */
            float: left;
            width: 33.33%;
            box-sizing: border-box;
            }

            .weatherContainer:after {
            content: "";
            display: table;
            clear: both;
            }

            .weatherRow {
            display: flex; 
            justify-content: space-between; 
            margin: 8px 0; 
            padding: 5px 0;
            border-bottom: 1px solid #ddd;
            }

            .label {
            font-family: "neueHaasBold"; 
            /* color: #0066cc; */
            }

            .value { 
            font-family: "neueHaas";
            }

            .status { 
            margin-top: 20px; 
            font-style: italic; 
            color: #666; 
            text-align: center;
            clear: both;
            width: 100%; 
            display: block; 
            padding: 10px 0; 
            }


            /* WEATHER DISPLAY END */

            /* FOOTER START */
            /* footer {
            position: absolute;
            bottom: 0%;
            left: 0;
            width: 100%;
            padding: 10px;
            background-color: rgba(255, 255, 255, 0.8);
            text-align: center;
            } */

            .footer {
            margin-top: 100px;
            width: 100%;
            padding: 100px, 15%;
            padding-top: 50px;
            padding-bottom: 50px;
            background-color: rgba(0, 46, 90, 1); /* rgb(37, 150, 190) */
            color: black;
            display: flex;
            font-family: 'NeueHaas';
            line-height: 1.65;
            }

            .footer div{
            text-align: center;
            }

            .memberName, h3 {
            color: white;
            font-family: 'NeueHaasBold';
            }

            .footer a {
            color: white;
            font-family: 'NeueHaas';
            }

            .NeueHaas {
            color: white;
            font-family: 'NeueHaas';
            }

            .col2 {
            flex: 0.25;
            }

            .col4 {
            flex: 0.25;
            }

            .col6 {
            flex: 0.25;
            }

            .col7 {
            flex: 0.25;
            }
            /* FOOTER END */

            /* MAIN PAGE END */
            /* ************************************************* */
        </style>
    </head>

    <body>

        <!-- Section 1: Menu Bar -->
        <input id="page-nav-toggle" class="main-navigation-toggle" type="checkbox" />
        <label for="page-nav-toggle">
          <svg class="icon--menu-toggle" viewBox="0 0 60 30">
            <g class="icon-group">
              <g class="icon--menu">
                <path d="M 6 0 L 54 0" />
                <path d="M 6 15 L 54 15" />
                <path d="M 6 30 L 54 30" />
              </g>
              <g class="icon--close">
                <path d="M 15 0 L 45 30" />
                <path d="M 15 30 L 45 0" />
              </g>
            </g>
          </svg>
        </label>
        
        <nav class="main-navigation">
          <ul>
            <li><a href="index.html" class="nav-link">Laxonar</a></li>
            <li><a href="sweep.html" class="nav-link">Visuals</a></li>
            <li><a href="https://www.vg.no/" class="nav-link">VG</a></li>
            <li><a href="index.html#footer" class="nav-link">Contact Us</a></li>
          </ul>
        </nav>

        <!-- Section 2: Header -->
        <header id="fixedHeader">
            <a href="index.html" class="logo-container">
                <img id="logo" src="laxonarLogoOnly.png" alt="Laxonar Logo">
                <span class="zilap-font">Laxonar</span>
            </a>
            <!-- <h2 class="zilap-font">Laxonar</h2> -->
        </header> 

        <!-- Section 3: Cover Image -->
        <section>
            <div class="backdrop" style="background-image: url(https://stiimaquacluster.no/wp-content/uploads/2023/03/Nautilus_komplett_illustrasjon_1_JR_2023_v2_16-9.png);" role="img"></div>

            <div class="inner-front-page">
                <div class="live-number-title">
                    Live Data 
                    <span class="blinking-dot">
                     </span></div> <!-- &nbsp; -->
                <br>
                <p id="sensorData"> Waiting for data...</p>
            </div>
        </section>

        <!-- Section 4: Fish Detection History -->
        <!-- Section 5: Graph -->
         <section class="content">
            <div class="detection-graph-container">
                <div class="fish-history-container">
                    <h2 class="fish-detection-history-title">Fish Detection History</h2>
                    <table id="fishHistoryTable">
                        <thead>
                            <tr>
                                <th id="fishNumber">Fish no.</th>
                                <th>Time</th>
                                <th>Date</th>
                                </tr>                             
                        </thead>

                        <tbody id="fishHistoryBody"> <!-- Data will automatically be updated in here --></tbody>
                    </table>
                </div>

                <div class="graph-container">
                    <h2 class="graph-title">Graph</h2>
                </div>
            </div>
        </section>

        <!-- Section 5: Weather Display -->
         <div class="weatherContainer">
            <h1 id="weatherH1">Complete Weather Dashboard</h1>
            <div class="weatherColumn">
                <h2 id="weatherH2">Current Weather</h2>
                <div class="weatherRow"><span class="label">Main: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Description: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Temperature: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Feels like: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Humidity: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Sea level pressure: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Ground level pressure: </span><span class="value"></span></div>
            </div>
            <div class="weatherColumn">
                <h2 id="weatherH2">Other</h2>
                <div class="weatherRow"><span class="label">Wind speed: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Wind direction: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Wind gust: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Cloudiness: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Visibility: </span><span class="value"></span></div>
            </div>
            <div class="weatherColumn">
                <h2 id="weatherH2">Location Information</h2>
                <div class="weatherRow"><span class="label">Time: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">City: </span><span class="value"></span></div>
                <div class="weatherRow"><span class="label">Country: </span><span class="value"></span></div>
            </div>
            <p class='status'>Last update: </p>
         </div>

        <!--- Section 8: Footer -->

        <div class="footer" id="footer">
            <div class="col7">
                <h3>Laxonar Technologies AS</h3>
                <p class="NeueHaas">Mortensine, Gamle Elektro</p>
            </div>

            <div class="col1">
                <p class="memberName">Johnny Ngo Nguyen</p>
                <a href="mailto:johnnynn@stud.ntnu.no">johnnynn@stud.ntnu.no</a>
            </div>

            <div class="col2">
                <p class="memberName">Eva Holm Skillingstad</p>
                <a href="mailto:fyll@stud.ntnu.no">evahsk@stud.ntnu.no</a>
            </div>

            <div class="col3">
                <p class="memberName">Håvard Lien Juvik</p>
                <a href="mailto:fyll@stud.ntnu.no">hljuvik@stud.ntnu.no</a>
            </div>

            <div class="col4">
                <p class="memberName">Gabija Liesyte</p>
                <a href="mailto:fyll@stud.ntnu.no">gabijal@stud.ntnu.no</a>
            </div>

            <div class="col5">
                <p class="memberName">Josefine Alice Helgen</p>
                <a href="mailto:fyll@stud.ntnu.no">jahelge@stud.ntnu.no</a>
            </div>

            <div class="col6">
                <p class="memberName">Natalia Chwiejczak</p>
                <a href="mailto:fyll@stud.ntnu.no">natalc@stud.ntnu.no</a>
            </div>
            <!-- <div class="col4">

            </div>
            <div class="col5">
                
            </div>
            <div class="col6">
                
            </div>
            <div class="col7">
                
            </div> -->
        </div>
        <!-- <footer>
            <p>Johnny Ngo Nguyen<br>
            <a href="mailto:johnnynn@stud.ntnu.no">johnnynn@stud.ntnu.no</a></p>

            <p>Johnny Ngo Nguyen<br>
                <a href="mailto:johnnynn@stud.ntnu.no">johnnynn@stud.ntnu.no</a></p>
        </footer> -->

        <script>
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
        </script>
    </body>
</html>
)rawliteral";

const int MAX_HISTORY_ITEMS = 200; // The limit of numbers to retrieve


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
                        FirebaseJsonData dateData;

                        entryJson.setJsonData(value.value); // This will parse the entry into a new JSON object

                        // Want to extract the counter and time values
                        entryJson.get(counterData, "counter");
                        entryJson.get(timeData, "time");
                        entryJson.get(dateData, "date");
                        
                        if(counterData.success && timeData.success && dateData.success) {
                            // Format the message to: HISTORY,counter,time,date
                            String historyMsg = "HISTORY, " + counterData.stringValue + ", " + timeData.stringValue+ "," + dateData.stringValue;
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


void sendWeatherToClients() {
    if (weatherDoc.size() > 0) {
        // Main weather data
        if (weatherDoc.containsKey("weather") && weatherDoc["weather"].size() > 0) {
            String weatherMain = weatherDoc["weather"][0]["main"].as<String>();
            String weatherDesc = weatherDoc["weather"][0]["description"].as<String>();
            
            webSocket.broadcastTXT("WEATHER_MAIN," + weatherMain);
            webSocket.broadcastTXT("WEATHER_DESC," + weatherDesc);
        }
        
        // Temperature and humidity data
        if (weatherDoc.containsKey("main")) {
            if (weatherDoc["main"].containsKey("temp")) {
                float temp = weatherDoc["main"]["temp"].as<float>();
                webSocket.broadcastTXT("WEATHER_TEMP," + String(temp, 2) + " °C");
            }
            
            if (weatherDoc["main"].containsKey("feels_like")) {
                float feelsLike = weatherDoc["main"]["feels_like"].as<float>();
                webSocket.broadcastTXT("WEATHER_FEELS," + String(feelsLike, 2) + " °C");
            }
            
            if (weatherDoc["main"].containsKey("humidity")) {
                int humidity = weatherDoc["main"]["humidity"].as<int>();
                webSocket.broadcastTXT("WEATHER_HUMIDITY," + String(humidity) + "%");
            }
            
            if (weatherDoc["main"].containsKey("sea_level")) {
                int seaLevel = weatherDoc["main"]["sea_level"].as<int>();
                webSocket.broadcastTXT("WEATHER_SEA_LEVEL," + String(seaLevel) + " hPa");
            }
            
            if (weatherDoc["main"].containsKey("grnd_level")) {
                int grndLevel = weatherDoc["main"]["grnd_level"].as<int>();
                webSocket.broadcastTXT("WEATHER_GRND_LEVEL," + String(grndLevel) + " hPa");
            }
        }
        
        // Wind data
        if (weatherDoc.containsKey("wind")) {
            if (weatherDoc["wind"].containsKey("speed")) {
                float windSpeed = weatherDoc["wind"]["speed"].as<float>();
                webSocket.broadcastTXT("WEATHER_WIND_SPEED," + String(windSpeed, 2) + " m/s");
            }
            
            if (weatherDoc["wind"].containsKey("deg")) {
                int windDeg = weatherDoc["wind"]["deg"].as<int>();
                webSocket.broadcastTXT("WEATHER_WIND_DEG," + String(windDeg) + "°");
            }
            
            if (weatherDoc["wind"].containsKey("gust")) {
                float windGust = weatherDoc["wind"]["gust"].as<float>();
                webSocket.broadcastTXT("WEATHER_WIND_GUST," + String(windGust, 2) + " m/s");
            }
        }
        
        // Clouds data
        if (weatherDoc.containsKey("clouds") && weatherDoc["clouds"].containsKey("all")) {
            int clouds = weatherDoc["clouds"]["all"].as<int>();
            webSocket.broadcastTXT("WEATHER_CLOUDS," + String(clouds) + "%");
        }
        
        // Visibility data
        if (weatherDoc.containsKey("visibility")) {
            float visibility = weatherDoc["visibility"].as<int>() / 1000.0;
            webSocket.broadcastTXT("WEATHER_VISIBILITY," + String(visibility, 2) + " km");
        }
        
        // Location information
        if (weatherDoc.containsKey("name")) {
            String cityName = weatherDoc["name"].as<String>();
            webSocket.broadcastTXT("WEATHER_CITY," + cityName);
        }
        
        if (weatherDoc.containsKey("sys") && weatherDoc["sys"].containsKey("country")) {
            String country = weatherDoc["sys"]["country"].as<String>();
            webSocket.broadcastTXT("WEATHER_COUNTRY," + country);
        }
        
        // Time information
        if (weatherDoc.containsKey("dt") && weatherDoc.containsKey("timezone")) {
            long dataTime = weatherDoc["dt"].as<long>();
            long timezone = weatherDoc["timezone"].as<long>();
            String dataTimeStr = getTimeString(dataTime, timezone);
            webSocket.broadcastTXT("WEATHER_TIME," + dataTimeStr);
        }
        
        // Status update
        webSocket.broadcastTXT("WEATHER_STATUS," + weatherData);
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
            Serial.println("Weather data received:");
            Serial.println(JSON_Data);
            
            // Parse JSON
            deserializeJson(weatherDoc, JSON_Data);
            
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

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.println("Client connected");
            if (weatherDoc.size() > 0) {
                sendWeatherToClients();
            }
            break;
        case WStype_DISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case WStype_TEXT:
            Serial.printf("Received: %s\n", payload);
            String message = String((char*)payload);

            if (message == "REQUEST_HISTORY") {
                sendHistoryToClient(num);
            } else if (message == "REQUEST_WEATHER") {
                // If client specifically requests weather data
                if (weatherDoc.size() > 0) {
                    sendWeatherToClients();
                } else {
                    getWeatherData();
                    sendWeatherToClients();
                }
            }
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

    getWeatherData();

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
                // strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo); // EDIT DATE AND TIME

                char dateString[20];
                char timeString[20];
                strftime(dateString, sizeof(dateString), "%Y-%m-%d", &timeinfo);
                strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
                Serial.printf("Counter: %d, Date: %s, Time: %s\n", counter, dateString, timeString);

                // Send counter + timestamp to WebSocket clients
                String message = String(counter) + " | " + String(timeString); 
                webSocket.broadcastTXT(message);

                // FIREBASE CODE
                if (Firebase.ready() && signupOK) {
                    // sendDataPrevMillis = millis(); // No longer needed as we dont want update every 5 seconds
                
                    String counterStr = String(counter);
                    String dateStr = String(dateString);
                    String timeStr = String(timeString);
                    // String dateTimeStr = String(timeString); // Convert timeString to String // EDIT DATE AND TIME

                    // Creates a unique key for each data entry
                    String dataHolderKey = String(millis()); // Using millis() for a more robust unique key

                    FirebaseJson jsonData;
                    jsonData.set("counter", counterStr);
                    jsonData.set("date", dateStr);
                    jsonData.set("time", timeStr);
                    // jsonData.set("time", dateTimeStr); // EDIT DATE AND TIME
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

    unsigned long currentMillis = millis();
    if (currentMillis - lastWeatherUpdate >= weatherUpdateInterval) {
        getWeatherData();
        sendWeatherToClients();
        lastWeatherUpdate = currentMillis;
    }
}