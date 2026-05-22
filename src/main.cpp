#include <Arduino.h>
#include <Wire.h>
#include <SPI.h> // O Remédio do PlatformIO
#include "MAX30105.h"

MAX30105 particleSensor;

// Variáveis de Filtro DC (As constantes estruturais)
float dcRed = 0;
float dcIR = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Erro: MAX30105 mudo.");
    while (1);
  }

  // Configuração raiz do MAX30105
  byte ledBrightness = 0x1F; // Potência segura (não cega o sensor)
  byte sampleAverage = 4;
  
  // MUDANÇA CRÍTICA: ledMode = 2 liga os DOIS LEDs internos (Red e IR)
  byte ledMode = 2; 
  
  byte sampleRate = 400; 
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
}

void loop() {
  // Lendo os dois canais simultaneamente
  uint32_t rawRed = particleSensor.getRed(); 
  uint32_t rawIR = particleSensor.getIR(); 

  // Trava de Ar: Se não tiver dedo, não suja a matemática
  if (rawRed < 10000) return;

  // ==========================================
  // MATEMÁTICA: CANAL 1 (VERMELHO - 660nm)
  // ==========================================
  // 1. Extração do DC (Filtro Passa-Baixa: Osso, Pele, Gordura)
  if (dcRed == 0) dcRed = rawRed;
  dcRed = (dcRed * 0.95) + (rawRed * 0.05);
  
  // 2. Extração do AC (Filtro Passa-Alta: O Pulso Sanguíneo)
  float acRed = rawRed - dcRed;

  // ==========================================
  // MATEMÁTICA: CANAL 2 (INFRAVERMELHO - 880nm)
  // ==========================================
  // 1. Extração do DC
  if (dcIR == 0) dcIR = rawIR;
  dcIR = (dcIR * 0.95) + (rawIR * 0.05);
  
  // 2. Extração do AC
  float acIR = rawIR - dcIR;

  // ==========================================
  // SAÍDA PARA O TELEPLOT (4 VARIÁVEIS LIMPAS)
  // ==========================================
  Serial.print(">DC_Red:");
  Serial.println(dcRed);
  
  Serial.print(">AC_Red:");
  Serial.println(acRed);
  
  Serial.print(">DC_IR:");
  Serial.println(dcIR);
  
  Serial.print(">AC_IR:");
  Serial.println(acIR);
}