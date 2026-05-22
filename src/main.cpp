#include <Arduino.h>
#include <Wire.h>
#include <SPI.h> 
#include "MAX30105.h"

MAX30105 particleSensor;

// Variáveis dos Canais
float dcRed = 0;
float dcIR = 0;
float smoothACRed = 0;
float smoothACIR = 0;

// ==========================================
// VARIÁVEIS DE ALGORITMO CLÍNICO (BPM)
// ==========================================
long ultimoTempoBeat = 0;
float bpmMedio = 0;
bool pulsoDetectado = false;
float limiarBatimento = 20.0; // Ignora ruídos menores que 20 de amplitude

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Erro: MAX30105 mudo.");
    while (1);
  }

  // Configuração raiz
  byte ledBrightness = 0x1F; 
  byte sampleAverage = 4;
  byte ledMode = 2; // Vermelho e IR
  byte sampleRate = 400; 
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
}

void loop() {
  uint32_t rawRed = particleSensor.getRed(); 
  uint32_t rawIR = particleSensor.getIR(); 

  // Trava de Ar: Zera a máquina de estado se o dedo for retirado
  if (rawRed < 10000) {
    pulsoDetectado = false;
    bpmMedio = 0;
    return;
  }

  // Processamento DC e AC
  if (dcRed == 0) dcRed = rawRed;
  dcRed = (dcRed * 0.95) + (rawRed * 0.05);
  float acRed = rawRed - dcRed;

  if (dcIR == 0) dcIR = rawIR;
  dcIR = (dcIR * 0.95) + (rawIR * 0.05);
  float acIR = rawIR - dcIR;

  // Filtro Anti-Farpas
  smoothACRed = (smoothACRed * 0.8) + (acRed * 0.2);
  smoothACIR = (smoothACIR * 0.8) + (acIR * 0.2);

  // ==========================================
  // MÁQUINA DE ESTADOS: DETECTOR DE BPM
  // ==========================================
  // 1. Identifica a subida da sístole na onda Infravermelha
  if (smoothACIR > limiarBatimento && !pulsoDetectado) {
    pulsoDetectado = true; // Trava o gatilho
    
    long tempoAtual = millis();
    long deltaT = tempoAtual - ultimoTempoBeat;

    // 2. Filtro Fisiológico: Entre 30 BPM (2000ms) e 200 BPM (300ms)
    if (deltaT > 300 && deltaT < 2000) {
      float bpmInstantaneo = 60000.0 / (float)deltaT;

      // 3. Suavização para o Painel
      if (bpmMedio == 0) bpmMedio = bpmInstantaneo; 
      bpmMedio = (bpmMedio * 0.90) + (bpmInstantaneo * 0.10);

      // Imprime o BPM no Teleplot!
      Serial.print(">BPM:");
      Serial.println(bpmMedio);
    }
    
    ultimoTempoBeat = tempoAtual;
  }

  // 4. Rearme do Gatilho: A onda precisa cruzar o zero para baixo para validar a próxima batida
  if (smoothACIR < 0) {
    pulsoDetectado = false;
  }

  // Mantemos a onda AC na tela para a banca ver a correlação física
  Serial.print(">AC_IR_Limpo:");
  Serial.println(smoothACIR);
}