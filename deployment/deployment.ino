#include <PDM.h>
#include "arm_math.h"
#include <ArduTFLite.h>
#include "sound_model.h"

#define SAMPLE_RATE 16000
#define FFT_SIZE 512
#define HOP_LENGTH 256
#define N_MELS 26
#define N_MFCC 13
#define FIXED_FRAMES 32
#define FMIN 0.0f
#define FMAX 8000.0f

#define MFCC_MEAN -12.178165
#define MFCC_STD 65.034790
#define CONFIDENCE_THRESHOLD 0.7f

const char* CLASSES[]={"Clap", "Snap", "Tap", "Silence"};
const int N_CLASSES=4;

#define PDM_BUFFER_SIZE 256
#define AUDIO_BUFFER_SIZE (FFT_SIZE+HOP_LENGTH)

volatile int16_t pdmBuffer[PDM_BUFFER_SIZE];
volatile bool newPDMData=false;
int16_t audioBuffer[AUDIO_BUFFER_SIZE];
int audioWritePos=0;
int samplesCollected=0;

static arm_rfft_fast_instance_f32 rfftInstance;
static arm_dct4_instance_f32 dct4Instance;
static arm_rfft_instance_f32 dctRfftInstance;
static arm_cfft_radix4_instance_f32 cfftInstance;
static float32_t frameF32[FFT_SIZE];
static float32_t hammingWindow[FFT_SIZE];
static float32_t fftOutput[FFT_SIZE];
static float32_t powerSpectrum[FFT_SIZE/2];
static float32_t melEnergies[N_MELS];
static float32_t logMelEnergies[N_MELS];
static float32_t dctInput[N_MELS];
static float32_t mfccCoeffs[N_MFCC];
static float32_t mfccMatrix[N_MFCC*FIXED_FRAMES];
static int frameCount=0;
static float32_t melFilterbank[N_MELS*(FFT_SIZE/2)];
static arm_matrix_instance_f32 melMatrix;
static arm_matrix_instance_f32 specMatrix;
static arm_matrix_instance_f32 melEnergyMatrix;

namespace {
  tflite::AllOpsResolver opsResolver;
  const tflite::Model* tfModel=nullptr;
  tflite::MicroInterpreter* tfInterp=nullptr;
  TfLiteTensor* inputTensor=nullptr;
  TfLiteTensor* outputTensor=nullptr;
}

constexpr int TENSOR_ARENA_SIZE=60*1024;
static uint8_t tensorArena[TENSOR_ARENA_SIZE];

void setup() {
  Serial.begin(115200);
  while(!Serial && millis()<3000); //3sec for Serial Approval
  Serial.println("Start Sound Classifier");
  buildHammingWindow(hammingWindow, FFT_SIZE);
  Serial.println("Hamming Window Ready");
  buildMelFilterbank(melFilterbank, N_MELS, FFT_SIZE/2, SAMPLE_RATE, FMIN, FMAX);
  arm_mat_init_f32(&melMatrix, N_MELS, FFT_SIZE/2, melFilterbank);
  Serial.println("Mel Filterbank Ready");
  arm_rfft_fast_init_f32(&rfftInstance, FFT_SIZE);
  arm_dct4_init_f32(&dct4Instance, &dctRfftInstance, &cfftInstance, N_MELS, N_MELS/2, 0.123f);
  Serial.println("CMSIS DSP RFFT & DCT Ready");
  tfModel=tflite::GetModel(sound_model_tflite);
  if(tfModel->version()!=TFLITE_SCHEMA_VERSION) {
    Serial.println("Error: Model schema version mismatching!");
    while(true);
  } 
  static tflite::MicroInterpreter staticInterpreter(tfModel, opsResolver, tensorArena, TENSOR_ARENA_SIZE);
  tfInterp=&staticInterpreter;
  if(tfInterp->AllocateTensors()!=kTfLiteOk) {
    Serial.println("Error: AllocateTensors() failed! Increase TENSOR_ARENA_SIZE");
    while(true);
  }
  inputTensor=tfInterp->input(0);
  outputTensor=tfInterp->output(0);
  Serial.println("TFLite model loaded");

  PDM.onReceive(onPDMData);
  if(!PDM.begin(1,SAMPLE_RATE)) {
    Serial.println("ERROR: PDM.begin() failed!");
    while(true);
  }
  Serial.println("Ready! Listening:");
}

void loop() {
  if(newPDMData) {
    noInterrupts();
    newPDMData=false;
    int16_t localBuf[PDM_BUFFER_SIZE];
    memcpy(localBuf, (void*)pdmBuffer, PDM_BUFFER_SIZE*sizeof(int16_t));
    interrupts();
    for(int i=0; i<PDM_BUFFER_SIZE; i++) {
      audioBuffer[audioWritePos%AUDIO_BUFFER_SIZE]=localBuf[i];
      audioWritePos++;
      samplesCollected++;
    }
    if(samplesCollected>=FFT_SIZE) {
      samplesCollected-=HOP_LENGTH;
      int startIdx=(audioWritePos-FFT_SIZE+AUDIO_BUFFER_SIZE)%AUDIO_BUFFER_SIZE;
      for(int i=0; i<FFT_SIZE; i++) {
        frameF32[i]=(float32_t)audioBuffer[(startIdx+i)%AUDIO_BUFFER_SIZE]/32768.0f;
      }
      arm_mult_f32(frameF32, hammingWindow, frameF32, FFT_SIZE);
      arm_rfft_fast_f32(&rfftInstance, frameF32, fftOutput, 0);
      arm_cmplx_mag_squared_f32(fftOutput, powerSpectrum, FFT_SIZE/2);
      arm_mat_init_f32(&specMatrix, FFT_SIZE/2, 1, powerSpectrum);
      arm_mat_init_f32(&melEnergyMatrix, N_MELS, 1, melEnergies);
      arm_mat_mult_f32(&melMatrix, &specMatrix, &melEnergyMatrix);
      for(int m=0; m<N_MELS; m++) {
        melEnergies[m]=fmaxf(melEnergies[m], 1e-10f);
      }
      arm_vlog_f32(melEnergies, logMelEnergies, N_MELS);
      arm_scale_f32(logMelEnergies, 4.342944819f, logMelEnergies, N_MELS);
      memcpy(dctInput, logMelEnergies, N_MELS*sizeof(float32_t));
      arm_dct4_f32(&dct4Instance, dctInput, dctInput);
      memcpy(mfccCoeffs, dctInput, N_MFCC*sizeof(float32_t));
      if(frameCount<FIXED_FRAMES) {
        for(int c=0; c<N_MFCC; c++) {
          mfccMatrix[c*FIXED_FRAMES+frameCount]=mfccCoeffs[c];
        }
        frameCount++;
      }
      if(frameCount>=FIXED_FRAMES) {
        frameCount=0;
        for(int i=0; i<N_MFCC*FIXED_FRAMES; i++) {
          mfccMatrix[i]=(mfccMatrix[i]-MFCC_MEAN)/MFCC_STD;
        }
        float* tflInput=inputTensor->data.f;
        for(int c=0; c<N_MFCC; c++) {
          for(int t=0; t<FIXED_FRAMES; t++) {
            tflInput[c*FIXED_FRAMES+t]=mfccMatrix[c*FIXED_FRAMES+t];
          }
        }
        if(tfInterp->Invoke()!=kTfLiteOk) {
          Serial.println("ERROR: Invoke() failed");
          return;
        }
        float* probs=outputTensor->data.f;
        int bestClass=0;
        float bestProb=probs[0];
        for(int i=1; i<N_CLASSES; i++) {
          if(probs[i]>bestProb) {
            bestProb=probs[i];
            bestClass=i;
          }
        }
        if(bestProb>=CONFIDENCE_THRESHOLD) {
          Serial.print(">");
          Serial.print(CLASSES[bestClass]);
          Serial.print("  (");
          Serial.print((int)(bestProb*100));
          Serial.print("%)");
        }
        else {
          Serial.println("... [low confidence - ignoring]");
        }
      }
    }
  }
}

void onPDMData() {
  int bytesAvailable=PDM.available();
  PDM.read((void*)pdmBuffer, bytesAvailable);
  newPDMData=true;
}

void buildHammingWindow(float32_t* win, int length) {
  for(int n=0; n<length; n++) {
    win[n]=0.54f-0.46f*cosf(2.0f*PI*n/(length-1));
  }
}

float32_t hzToMel(float32_t hz) {
  return 2595.0f*log10f(1.0f+hz/700.0f);
}

float32_t melToHz(float32_t mel) {
  return 700.0f*(powf(10.0f,mel/2595.0f)-1.0f);
}

void buildMelFilterbank(float32_t* filterbank, int nMels, int nFFTBins, int sampleRate, float32_t fMin, float32_t fMax) {
  memset(filterbank, 0, nMels*nFFTBins*sizeof(float32_t));
  float32_t melMin=hzToMel(fMin);
  float32_t melMax=hzToMel(fMax);
  float32_t melPoints[N_MELS+2];
  for(int m=0; m<=nMels+1; m++) {
    melPoints[m]=melMin+(float32_t)m*(melMax-melMin)/(nMels+1);
  }
  float32_t hzPoints[N_MELS+2];
  int binPoints[N_MELS+2];
  for(int m=0; m<=nMels+1; m++) {
    hzPoints[m]=melToHz(melPoints[m]);
    binPoints[m]=(int)floorf((FFT_SIZE+1)*hzPoints[m]/sampleRate);
    binPoints[m]=constrain(binPoints[m], 0, nFFTBins-1);
  }
  for(int m=1; m<=nMels; m++) {
    int lo=binPoints[m-1];
    int ctr=binPoints[m];
    int hi=binPoints[m+1];
    for(int k=lo; k<ctr; k++) {
      float32_t denom=(float32_t)(ctr-lo);
      filterbank[(m-1)*nFFTBins+k]=(denom>0)?(float32_t)(k-lo)/denom:0.0f;
    }
    for(int k=ctr; k<=hi; k++) {
      float32_t denom=(float32_t)(hi-ctr);
      filterbank[(m-1)*nFFTBins+k]=(denom>0)?(float32_t)(hi-k)/denom:0.0f;
    }
  }
}

void printProbabilites(float* probs) {
  Serial.println("Probabilities:");
  for(int i=0; i<N_CLASSES; i++) {
    Serial.print(CLASSES[i]);
    Serial.print(":");
    Serial.print(probs[i]*100.0f, 1);
    Serial.println("%");
  }
}