#include <Wire.h>
#include "HX711.h"

HX711 scale1;
HX711 scale2;

#define HALL_PIN_D0 2
#define PULSE_THRESHOLD 20

volatile int pulseCount = 0;
unsigned long lastTime = 0; // Время последнего считывания
unsigned long startTime = 0; // Время начала накопления импульсов
float rpm = 0; // Обороты

void setup() {
  Serial.begin(115200);
  Serial.println("Start config...");

  // Инициализация тензодатчиков
  scale1.begin(4, 3);
  scale1.set_scale(108);
  scale1.tare();

  scale2.begin(6, 5);
  scale2.set_scale(105);
  scale2.tare();
  
  Serial.println("Tenz configurated");

  // Инициализация датчика Холла
  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Hall configurated");
}

void loop() {
  // Если набрано PULSE_THRESHOLD импульсов, рассчитываем обороты
  if (pulseCount >= PULSE_THRESHOLD) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    // Рассчитываем обороты (RPM)
    rpm = (pulseCount * 60000.0) / (elapsedTime * PULSE_THRESHOLD);
    pulseCount = 0;
    startTime = currentTime;

    // Считываем данные с тензодатчиков
    float moment = scale1.get_units(10) * 0.001;  // Момент
    float thrust = scale2.get_units(10);  // Тяга

    // Выводим обороты, момент и тягу в формате:
    Serial.print("Момент:");
    Serial.print(moment, 4);  // 4 знака после запятой
    Serial.print(":Тяга:");
    Serial.print(thrust, 4);
    Serial.print(":Об/мин:");
    Serial.println(rpm);
  }
}

// Функция прерывания для подсчета импульсов с датчика Холла
void countPulse() {
  pulseCount++;
}