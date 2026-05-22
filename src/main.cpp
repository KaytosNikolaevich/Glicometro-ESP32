#include <Arduino.h>
#include <Wire.h>
#include <SPI.h> 
#include "MAX30105.h"

MAX30105 particleSensor;

// Variáveis de Filtro DC (Base estrutural)
float dcRed = 0;
float dcIR = 0;

// Variáveis de Suavização AC (Filtro anti-farpas)
float smoothACRed = 0;
float smoothACIR = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("Erro: MAX30105 mudo.");
    while (1);
  }

  // Configuração raiz do MAX30105
  byte ledBrightness = 0x1F; 
  byte sampleAverage = 4;
  byte ledMode = 2; // Ligando Vermelho e IR
  byte sampleRate = 400; 
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
}

void loop() {
  uint32_t rawRed = particleSensor.getRed(); 
  uint32_t rawIR = particleSensor.getIR(); 

  // Trava de Ar
  if (rawRed < 10000) return;

  // ==========================================
  // CANAL 1 (VERMELHO - 660nm)
  // ==========================================
  if (dcRed == 0) dcRed = rawRed;
  dcRed = (dcRed * 0.95) + (rawRed * 0.05);
  float acRed = rawRed - dcRed;

  // ==========================================
  // CANAL 2 (INFRAVERMELHO - 880nm)
  // ==========================================
  if (dcIR == 0) dcIR = rawIR;
  dcIR = (dcIR * 0.95) + (rawIR * 0.05);
  float acIR = rawIR - dcIR;

  // ==========================================
  // FILTRO DE SUAVIZAÇÃO (AMORTECEDOR AC)
  // ==========================================
  // O fator 0.8 mantém a inércia da onda, o 0.2 aceita o dado novo.
  // Isso "passa a ferro" os espinhos do gráfico.
  smoothACRed = (smoothACRed * 0.8) + (acRed * 0.2);
  smoothACIR = (smoothACIR * 0.8) + (acIR * 0.2);

  // ==========================================
  // SAÍDA PARA O TELEPLOT
  // ==========================================
  Serial.print(">DC_Red:");
  Serial.println(dcRed);
  
  Serial.print(">AC_Red_Limpo:");
  Serial.println(smoothACRed);
  
  Serial.print(">DC_IR:");
  Serial.println(dcIR);
  
  Serial.print(">AC_IR_Limpo:");
  Serial.println(smoothACIR);
}