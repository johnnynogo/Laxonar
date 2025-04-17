#include <Arduino.h>

// Pin definitions
const int trigPin = 17;  // Trigger pin connected to GPIO 17
const int echoPin = 16;  // Echo pin connected to GPIO 16

// Constants
const long baud = 115200;
const float soundSpeed = 0.034;  // Speed of sound in cm/microsecond
const float cmToMm = 10.0;       // Conversion from cm to mm

// Variables
volatile unsigned long pulseStart = 0;
volatile unsigned long pulseEnd = 0;
volatile boolean newData = false;
volatile float distance = 0;  // Distance in mm

void setup() {
  // Initialize Serial Monitor
  Serial.begin(baud);
  Serial.println("HC-SR04 Ultrasonic Sensor Test");
  
  // Set pin modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Ensure trigger pin is LOW
  digitalWrite(trigPin, LOW);
  delay(500);
}

void loop() {
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Set the trigger pin HIGH for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echo pin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate distance
  // Distance = (Time x Speed of sound) / 2 (divided by 2 because sound travels to object and back)
  float distanceCm = duration * soundSpeed / 2.0;
  float distanceMm = distanceCm * cmToMm;
  
  // Print results
  Serial.print("Distance: ");
  Serial.print(distanceMm);
  Serial.println(" mm");
  
  // Wait before next measurement
  delay(100);
}