#include <PDM.h>

#define EXTERNAL_PDM_CLK D2 //P1.11
#define EXTERNAL_PDM_DATA D3 //P1.12

short sampleBuffer[256];
volatile int samplesRead;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  pinMode(EXTERNAL_PDM_CLK, OUTPUT);
  pinMode(EXTERNAL_PDM_DATA, INPUT);
  PDM.onReceive(onPDMdata);
  if(!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM!");
    while(1);
  }
  NRF_PDM->ENABLE=0;
  NRF_PDM->PSEL.CLK=digitalPinToPinName(EXTERNAL_PDM_CLK);
  NRF_PDM->PSEL.DIN=digitalPinToPinName(EXTERNAL_PDM_DATA);
  NRF_PDM->ENABLE=1;
  NRF_PDM->TASKS_START=1;
 
  PDM.setGain(40);
  Serial.println("External PDM Mic Initialized");
}

void loop() {
  if(samplesRead>0) {
    Serial.print(sampleBuffer[0]);
    Serial.print(" ");
    samplesRead=0;
  }
}

void onPDMdata() {
  int bytesAvailable=PDM.available();
  if(bytesAvailable>0) {
    PDM.read(sampleBuffer, bytesAvailable);
    samplesRead=bytesAvailable/2;
  }
}
