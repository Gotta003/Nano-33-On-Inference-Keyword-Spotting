#include <PDM.h>

static short sampleBuffer[512];
volatile int samplesRead = 0;
volatile int discardCount=0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  PDM.onReceive(onPDMdata);
  PDM.setBufferSize(512);
  if(!PDM.begin(1,16000)) {
    Serial.println("Failed to start PDM!");
    while(1);
  }
  NRF_PDM->ENABLE=0;
  NRF_PDM->PSEL.CLK=43;
  NRF_PDM->PSEL.DIN=44;
  NRF_PDM->MODE=0x02;
  NRF_PDM->ENABLE=1;
  NRF_PDM->TASKS_START=1;
  PDM.setGain(0); // Start with some gain
 
  delay(500);
  Serial.println("PDM Started");
}

void loop() {
  if (samplesRead > 0) {
    for (int i = 0; i < samplesRead; i++) {
      Serial.print(sampleBuffer[i]);
      Serial.print(" ");
    }
    samplesRead = 0;
  }
}

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  if(discardCount<50) {
    discardCount++;
    return;
  }
  samplesRead = bytesAvailable / 2;
}