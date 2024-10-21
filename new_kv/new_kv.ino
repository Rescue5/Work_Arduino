#include <Servo.h>

#define BUTTON_PIN 3
#define ESC_PIN A3
#define VOLTAGE_PIN A1
#define CURRENT_PIN A2

#define MAGNET_COUNT 7
#define HALL_PIN_D0 13 

// Пины для светодиодов
const int ledPins[6] = {4, 5, 6, 7, 8, 9}; // Светодиоды для выбора KV
const int resultLedPins[3] = {10, 11, 12}; // Светодиоды для результата
const int KV[6] = {300, 600, 900, 1200, 1500, 1800};


Servo esc;
int currentThrottle = 1000;
int selectedKVled = 0;
int selectedKV = 300;
float measuredKV = 0.0;
bool testRunning = false;
unsigned long buttonPressTime = 0;
bool buttonHeld = false;

int minThrottle = 1000;
int maxThrottle = 2000;

unsigned long testStartTime = 0;
const unsigned long accelTime = 10000; // Время ускорения
const unsigned long holdTime = 8000;   // Время удержания
const unsigned long decelTime = 5000;  // Время замедления

volatile int pulseCount = 0;
float rpm = 0;
float voltage = 0;
float current = 0;
float maxKV = 0;
bool isHoldTimeActive = false;
float holdTimeCurrentSum = 0.0;
int holdTimeCurrentCount = 0;

const float REFERENCE_VOLTAGE = 5.0;
const float MAX_INPUT_VOLTAGE = 25.0;
const float DIVIDER_RATIO = MAX_INPUT_VOLTAGE / REFERENCE_VOLTAGE;

void setup() {
  Serial.begin(9600);
  esc.attach(ESC_PIN);
  esc.writeMicroseconds(minThrottle);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  for (int i = 0; i < 6; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  for (int i = 0; i < 3; i++) {
    pinMode(resultLedPins[i], OUTPUT);
  }

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);
}

void loop() {
  // Управление светодиодами и выбором KV
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis(); // Начало удержания кнопки
    }
    
    if (!buttonHeld && millis() - buttonPressTime > 2000) {
      // Если кнопка удерживалась более 2 секунд, запускаем тест
      startTest();
      buttonHeld = true;
    }
  } else if (buttonPressTime > 0) {
    if (millis() - buttonPressTime < 2000) {
      // Короткое нажатие, переключаем KV
      selectedKVled = (selectedKVled + 1) % 6;
      selectedKV = KV[selectedKVled];
      updateKVSelection();
    }
    buttonPressTime = 0;
    buttonHeld = false;
  }

  if (testRunning) {
    runTest();
  }
}

void updateKVSelection() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(ledPins[i], i == selectedKVled ? HIGH : LOW);
  }
}

void startTest() {
  testRunning = true;
  measuredKV = 0.0;
  testStartTime = millis();

  holdTimeCurrentSum = 0.0;
  holdTimeCurrentCount = 0;
  isHoldTimeActive = false;
  maxKV = 0;
}

void runTest() {
  unsigned long elapsedTime = millis() - testStartTime;

  if (elapsedTime < accelTime) {
    currentThrottle = map(elapsedTime, 0, accelTime, 1000, 2000);  // Ускорение
  } else if (elapsedTime < accelTime + holdTime) {
    currentThrottle = 2000;  // Удержание на максимальных оборотах
    isHoldTimeActive = true;
    updateMeasurements(); // Обновляем измерения только на максимальных оборотах

    // Вычисляем KV только на максимальных оборотах
    if (voltage != 0) {
      float kv = rpm / voltage;
      if (kv > maxKV) {
        maxKV = kv;  // Сохраняем максимальное значение KV
      }
    }
    
  } else if (elapsedTime < accelTime + holdTime + decelTime) {
    currentThrottle = map(elapsedTime, accelTime + holdTime, accelTime + holdTime + decelTime, 2000, 1000);  // Замедление
    isHoldTimeActive = false;
  } else {
    testRunning = false;
    if (holdTimeCurrentCount > 0) {
      float avgCurrent = holdTimeCurrentSum / holdTimeCurrentCount;
      measuredKV = maxKV; // Окончательное значение KV
      Serial.print("Max KV: ");
      Serial.println(maxKV);
      Serial.print("Avg Current: ");
      Serial.println(avgCurrent);
    }
    evaluateResult();
  }

  esc.writeMicroseconds(currentThrottle);
  Serial.print("Скорость установлена на ");
  Serial.print(currentThrottle);
  Serial.print("\n");
}

void resetTest() {
  esc.writeMicroseconds(1000); // Остановка двигателя
  for (int i = 0; i < 3; i++) {
    digitalWrite(resultLedPins[i], LOW);
  }
}

void updateMeasurements() {
  noInterrupts();
  int pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  float revolutions = pulses / float(MAGNET_COUNT);
  rpm = revolutions * 60 * 0.9859;

  int sensorValue = analogRead(VOLTAGE_PIN);
  voltage = sensorValue * (REFERENCE_VOLTAGE / 1020.0) * DIVIDER_RATIO;

  int currentSensorValue = analogRead(CURRENT_PIN);
  float voltageAtCurrentPin = currentSensorValue * (REFERENCE_VOLTAGE / 1023.0);
  current = (voltageAtCurrentPin - 2.5) / 0.066;

  if (isHoldTimeActive) {
    holdTimeCurrentSum += current;
    holdTimeCurrentCount++;

    if (voltage != 0) {
      float kv = rpm / voltage;
      if (kv > maxKV) {
        maxKV = kv;
      }
    }
  }
}

void countPulse() {
  pulseCount++;
}

void evaluateResult() {
  float difference = abs((measuredKV - selectedKVled) / selectedKVled) * 100;

  if (difference <= 5) {
    digitalWrite(resultLedPins[0], HIGH); // Менее 5%
  } else if (difference <= 10) {
    digitalWrite(resultLedPins[1], HIGH); // Менее 10%
  } else {
    digitalWrite(resultLedPins[2], HIGH); // Более 10%
  }
}