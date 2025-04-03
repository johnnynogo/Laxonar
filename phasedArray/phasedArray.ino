// Transmitter fra Murata blir brukt: MA40S4S
// Merk: tidligere versjon av ESP32 boards må brukes
// ellers funker ikke transmitter
// Boards Manager -> søk 'Arduino ESP32 Boards' -> velg versjon 2.0.18

#include "Arduino.h"
#include "driver/ledc.h"
#include "esp32-hal-ledc.h"

const float threshold = 1.0; // RMS threshold for 0.1V peak 
// Ting er sensitivt, endre til 0.05V dersom man ikke får noen målinger

const float Vref = 5.0; // Assuming 5V ADC reference
const int ADC_res = 1024;
const float period = 1.0 / 40000.0; // Period for 40 kHz
const int samples = 80; // Adjusted for 80kHz sampling rate

const int sender1 = 6; // Sender pin, ytterst venstre
const int sender2 = 7; // Sender pin, midten
// const int sender3 = 8; // Sender pin, ytterst høyre
const int receiver = 4; // Mottakker pin
const int baud = 115200;

void setup() {
  Serial.begin(baud);
  
  // Arduino ESP32 PWM setup
  ledcSetup(0, 40000, 8); // Channel 0, 40 kHz, 8-bit resolution 
  ledcAttachPin(sender1, 0); // Attach pin to channel 

  ledcSetup(1, 40000, 8); // Channel 0, 40 kHz, 8-bit resolution 
  ledcAttachPin(sender2, 1); // Attach pin to channel 

  // ledcSetup(2, 40000, 8); // Channel 0, 40 kHz, 8-bit resolution 
  // ledcAttachPin(sender3, 2); // Attach pin to channel 

  ledcWrite(0, 128); // 50% duty cycle

  // For phased array
  // Diameter på transmitter/receiver er 9.9+-0.3mm. deltaT = (p * sin(theta)) / c, hvor c = 343m/s (i luft)
  
  pinMode(receiver, INPUT);
}

void loop() {
  float sumSq = 0.0;
  unsigned long startTime = micros();
  
  for (int i = 0; i < samples; i++) {
    int raw = analogRead(receiver);
    float voltage = (raw / (float)ADC_res) * Vref;
    sumSq += voltage * voltage;
    
    // Ensure consistent sampling rate
    while (micros() - startTime < (i + 1) * (1000000 / 80000)); 
  }
  
  float Vrms = sqrt(sumSq / samples);
  Serial.print("Vrms: ");
  Serial.println(Vrms);
  
  if (Vrms > threshold) {
    Serial.println("Peak voltage above 0.1V detected!");
  }
}