#include <Wire.h>
#include "HX711.h"

HX711 scale1;
HX711 scale2;

#define HALL_PIN_D0 2

// Количество магнитов на диске
#define MAGNET_COUNT 2

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 500;  // Интервал измерений в миллисекундах
float rpm = 0;

int prevMoment = 0;
int prevThrust = 0;
int prevRpm = 0;

float Moment = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Start config...");

  // Инициализация тензодатчиков
  scale1.begin(4, 3);
  scale1.set_scale(108);
  scale1.tare();

  scale2.begin(8, 7);
  scale2.set_scale(105);
  scale2.tare();
  
  Serial.println("Tenz configurated");

  // Инициализация датчика Холла
  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Hall configurated");
}

void loop() {
  unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

    float kg = scale1.get_units()*0.001;
    float moment = kg * 2.0566;  // Момент
    float thrust = scale2.get_units() * 0.001;  // Тяга

    previousMillis = currentMillis;
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    // Переводим импульсы в полные обороты в минуту (RPM)
    float revolutions = pulses / float(MAGNET_COUNT);  // Количество полных оборотов за секунду
    rpm = revolutions * 120;                            // Переводим в обороты за минуту

    if (moment < 0){
      moment = 0;
    }

    if (thrust < 0){
      thrust = 0;
    }


    if ((moment != prevMoment || thrust != prevThrust || rpm != prevRpm) && (moment > 0.001 || thrust >0.005 || rpm>100)){

      Serial.print("Момент:");
      Serial.print(moment, 4);  // 4 знака после запятой
      Serial.print("Кг:");
      Serial.print(kg);
      Serial.print(":Тяга:");
      Serial.print(thrust, 4);
      Serial.print(":Об/мин:");
      Serial.println(rpm,0);

    }
    prevMoment = moment;
    prevThrust = thrust;
    prevRpm = rpm;
  }

}

// Функция прерывания для подсчета импульсов с датчика Холла
void countPulse() {
  //Serial.println("pulse");
  pulseCount++;
}