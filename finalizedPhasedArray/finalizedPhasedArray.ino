#include <driver/adc.h>




const long baud = 921600;
const unsigned long cyclesPerSecond = 240000000;
const unsigned long cyclesPerMilliSecond = cyclesPerSecond / 1000;
const unsigned long cyclesPerMicroSecond = cyclesPerSecond / 1000000;
const unsigned long cyclesPerPhase = cyclesPerSecond / 40000;
const unsigned long cyclesPerPhaseHalf = cyclesPerPhase / 2;




const float meterPerSecond = 344.f;
const float mmPerUs = meterPerSecond * 0.001f;
const float startMm = 20; //Endret
const long startUs = long(startMm / mmPerUs);




const int pinCount = 8;
const int pulseLength = 20;
const int pins[] = {9,8,7,6,5,4,3,2}; //Rekkefølge er viktig for rett faseforskyvningsrekkefølge
volatile int phaseShift = 0;
const int samplingRate = 20000;
volatile int depth = 256;
volatile int width = 64;
int phaseShifts[64][pinCount]; //Første [n] (som nå er [64]) skal være lik width
volatile int currentRec = 0;
volatile bool ready2send = false;
int distance[2];
int start[2];
int recShift[2];
short rec[2][64][256];
unsigned long nullmask = 0;




void print4(unsigned long v)
{
 const char *hex = "0123456789abcdef";
 Serial.write(hex[v & 15]);
}




void print8(unsigned long v)
{
 print4(v >> 4);
 print4(v);
}




void print16(unsigned long v)
{
 print8(v >> 8);
 print8(v);
}








void comTask(void *data)
{
 while(true)
 {
   do
   {
     delay(1);
   }
   while(!ready2send);
   print16(width);
   print16(depth);
   print16(start[currentRec ^ 1]);
   print16(distance[currentRec ^ 1]);
   for(int j = 0; j < width; j++)
   {
     for(int i = 0; i < depth; i++)
     {
       print8(min((rec[currentRec ^ 1][j][i] * (i + 8)) / 64, 255));
     }
   }
   Serial.println();
   static String s = "";
   while(Serial.available() > 0)
   {
   char ch = Serial.read();
   if(ch == '\n')
   {
     sscanf(s.c_str(), "%d %d", &width, &depth);
     s = "";
     Serial.println("OK");
   }
   else
     s += ch;
   }
   ready2send = false;
 }
}




void waveTask(void *param)
{
 long sum = 0;
 long avg = 2048;
 while(true)
 {
   //send puls 400µs
   unsigned long t = 0;
   unsigned long ot = ESP.getCycleCount();
   while(true)
   {
     unsigned long ct = ESP.getCycleCount();
     unsigned long dt = ct - ot;
     ot = ct;
     t += dt;
     if(t >= 400 * cyclesPerMicroSecond) break;
     unsigned long phase = t % cyclesPerPhase;
     unsigned long parallel1 = 0;
     unsigned long parallel0 = 0;
     for(int i = 0; i < pinCount; i++)
     {
       if((phaseShifts[phaseShift][i] + phase) % cyclesPerPhase > cyclesPerPhaseHalf)
         parallel1 |= 1 << pins[i];
       else
         parallel0 |= 1 << pins[i];/**/
     }
     *(unsigned long*)GPIO_OUT_W1TS_REG = parallel1;
     *(unsigned long*)GPIO_OUT_W1TC_REG = parallel0;
   }
   *(unsigned long*)GPIO_OUT_W1TC_REG = nullmask;
   //read echo
   //wait 800µs
   delayMicroseconds(startUs);
   adc1_config_width(ADC_WIDTH_BIT_13); //13 bit for ESP32S2
   adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); 
   recShift[currentRec] = phaseShift;
   unsigned long t0 = ESP.getCycleCount();
   for(int i = 0; i < depth; i++)
   {
     int a = adc1_get_raw(ADC1_CHANNEL_0);
     sum += a;
     rec[currentRec][phaseShift][i] = abs(a - avg);
   }
   distance[currentRec] = (int)((ESP.getCycleCount() - t0) * mmPerUs / cyclesPerMicroSecond);
   start[currentRec] = startMm;
   phaseShift = (phaseShift + 1) % width;
   if(phaseShift == 0)
   {
     do{delay(0);} while(ready2send);
     currentRec ^= 1;
     ready2send = true;
     while (ready2send) delay(1);
     avg = (sum / depth) / width;
     sum = 0;
   }
 }
}




void calculateWave()
{
 for(int i = 0; i < width; i++)
 {
   float shift = (31 - i) / 64.0f;
   for(int j = 0; j < pinCount; j++)
   {
     float phase = j * shift;
     phase = fmod(phase, 1.0f); // fmod håndterer både pos/neg
     if (phase < 0.0f) {
         phase += 1.0f;
     }
     phaseShifts[i][j] = (int)(phase * cyclesPerPhase);
   }
 }
}




void setup()
{
 Serial.begin(baud);
 Serial.println("Phased array setup!");
 for(int i = 0; i < pinCount; i++){
   pinMode(pins[i], OUTPUT);
 }
 for (int i = 0; i < pinCount; i++) {// Bygger nullmasken ut fra hvilke pins som brukes
   nullmask |= (1UL << pins[i]); 
   pinMode(pins[i], OUTPUT);
 }
 calculateWave();
 TaskHandle_t xHandle = NULL;
 // xTaskCreatePinnedToCore(comTask, "Communication", 10000, 0, 2, &xHandle, 0);
 // xTaskCreatePinnedToCore(waveTask, "Wave", 10000, 0,  ( 2 | portPRIVILEGE_BIT ), &xHandle, 1);
 xTaskCreate(comTask, "Communication", 10000, 0, 1, NULL);  // Lav prioritet
 xTaskCreate(waveTask, "Wave", 10000, 0, 3, NULL); //Høyere prioritet
}




void loop()
{
 delay(1000);
}


