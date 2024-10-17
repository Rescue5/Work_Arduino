#include <Wire.h>
#include "HX711.h"
#include <Servo.h>
#include <avr/sleep.h>

HX711 scale1;
HX711 scale2;

const int numReadings = 10;
float readings1[numReadings];
float readings2[numReadings];
float Moment = 0;

int readIndex = 0;
float total1 = 0;
float total2 = 0;

#define HALL_PIN_D0 2
#define MAGNET_COUNT 7

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 1000;
float rpm = 0;

Servo esc;
int escPin = A1;
int minThrottle = 1000;
int maxThrottle = 1500;
int currentSpeed = minThrottle;

bool testInProgress = false;
bool stopCommand = false;

unsigned long speedUpdateMillis = 0;
const int speedUpdateInterval = 100;  // Интервал для обновления скорости (0.1 сек)
unsigned long testStartTime = 0;  // Время старта теста
bool accelerating = false;
bool holdingMaxSpeed = false;
bool decelerating = false;

void setup() {
  Serial.begin(115200);
  // Serial.println("Start config...");

  scale1.begin(3, 4);
  scale1.set_scale(850);
  scale1.tare();
  scale2.begin(5, 6);
  scale2.set_scale(109);
  scale2.tare();

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings1[thisReading] = 0;
    readings2[thisReading] = 0;
  }

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);
}

void loop() {
  // Проверка команд через Serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command == "START" && !testInProgress) {
      Serial.println("START");
      startTest();
    } else if (command == "STOP") {
      stopCommand = true;
    } else if (command == "TEST") {
      Serial.println("OK");
    }
  }

  // Если тест в процессе выполнения
  if (testInProgress) {
    unsigned long currentMillis = millis();

    // Управление ускорением, поддержанием и торможением
    if (accelerating && currentMillis - speedUpdateMillis >= speedUpdateInterval) {
      speedUpdateMillis = currentMillis;
      if (currentSpeed < maxThrottle) {
        currentSpeed += 10;
        if (currentSpeed > maxThrottle) currentSpeed = maxThrottle;
        setSpeed(currentSpeed);
      } else {
        accelerating = false;
        holdingMaxSpeed = true;
        speedUpdateMillis = currentMillis;
      }
    }

    // Удержание максимальной скорости
    if (holdingMaxSpeed && currentMillis - speedUpdateMillis >= 1000) {
      holdingMaxSpeed = false;
      decelerating = true;
    }

    // Торможение
    if (decelerating && currentMillis - speedUpdateMillis >= speedUpdateInterval) {
      speedUpdateMillis = currentMillis;
      if (currentSpeed > minThrottle) {
        currentSpeed -= 10;
        if (currentSpeed < minThrottle) currentSpeed = minThrottle;
        setSpeed(currentSpeed);
      } else {
        decelerating = false;
        testInProgress = false;
        Serial.println("Test complete.");
      }
    }

    // Логирование данных с датчиков
    logSensors();  // Вызов logSensors для записи данных

    // Проверка команды экстренной остановки
    if (stopCommand) {
      emergencyStop();
    }
  }
}

void startTest() {
  Serial.println("Test started.");
  testStartTime = millis();  // Сохраняем время старта теста
  testInProgress = true;
  stopCommand = false;
  currentSpeed = minThrottle;
  accelerating = true;
  decelerating = false;
  holdingMaxSpeed = false;
  speedUpdateMillis = millis();
}

void emergencyStop() {
  Serial.println("Emergency stop activated. Slowing down...");
  
  // Плавное снижение скорости до 1000 (минимума)
  while (currentSpeed > minThrottle) {
    currentSpeed -= 10;
    if (currentSpeed < minThrottle) currentSpeed = minThrottle;
    setSpeed(currentSpeed);
    delay(100);
  }
  
  testInProgress = false;
  stopCommand = false;
  Serial.println("Motor stopped.");
}

void setSpeed(int speed) {
  esc.writeMicroseconds(speed);
  Serial.print("Speed set to: ");
  Serial.println(speed);
}

void logSensors() {
  // Проверка, что тест выполняется
  if (!testInProgress) return;

  total1 = total1 - readings1[readIndex];
  total2 = total2 - readings2[readIndex];

  readings1[readIndex] = scale1.get_units() * 0.001;
  readings2[readIndex] = scale2.get_units() * 0.001;

  total1 = total1 + readings1[readIndex];
  total2 = total2 + readings2[readIndex];

  readIndex = (readIndex + 1) % numReadings;

  float average1 = (total1 / numReadings);
  float average2 = (total2 / numReadings);

  if (average1 < 0.080) average1 = 0;
  if (average2 < 0.080) average2 = 0;

  Moment = 2.0566 * average1;
  if (Moment < 0.0050) Moment = 0;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float revolutions = pulses / float(MAGNET_COUNT);
    rpm = revolutions * 60 * 0.9859;
  }

  unsigned long elapsedTime = millis() - testStartTime;  // Прошедшее время с начала теста
  float elapsedTimeSeconds = elapsedTime / 1000.0;  // Переводим в секунды

  // Логирование данных с датчиками и временем
  if (average1 > 0.020 || Moment > 0.010 || (rpm > 300 && rpm < 25000 && average2 > 0.080)) {
    Serial.print("Текущее время:");
    Serial.print(millis() / 1000.0, 2);  // Текущее время в секундах
    Serial.print(":Время с начала теста:");
    Serial.print(elapsedTimeSeconds, 2);  // Время с начала теста в секундах

    Serial.print(":Момент:");
    Serial.print(average1, 4);
    Serial.print("*");
    Serial.print(Moment, 4);

    Serial.print(":Тяга:");
    Serial.print(average2, 4);

    Serial.print(":Об/мин:");
    Serial.print(rpm, 4);
    Serial.print("\n");
  }
}