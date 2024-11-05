#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define HALL_PIN_D0 2
#define VOLTAGE_PIN A6
#define CURRENT_PIN A5
#define BUTTON_PIN_1 3
#define BUTTON_PIN_2 A2

#define MAGNET_COUNT 7
const float REFERENCE_VOLTAGE = 5.0;
const float MAX_INPUT_VOLTAGE = 25.0;
const float DIVIDER_RATIO = MAX_INPUT_VOLTAGE / REFERENCE_VOLTAGE;

const int ledPins[6] = {7, 8, 9, 10, 11, 12}; // Светодиоды для выбора KV
const int resultLedPins[3] = {6, 5, 4}; // Светодиоды для результата
const int KV[6] = {405, 601, 760, 857, 1080, 1270};
int selectedKVled = 0;
int selectedKV = 300;

bool testEnds = false;

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 1000;
float voltage = 0;
float rpm = 0;
float current = 0;
float maxKV = 0;
float maxKV_AverageCurrent = 0;
char currentStr[10];

const int numReadings = 2;
float currentReadings[numReadings] = {0.0};
int currentReadingIndex = 0;
float totalCurrent = 0.0;
float averageCurrent = 0.0;

Servo esc;
int escPin = A3;
int minThrottle = 1000;
int maxThrottle = 2000;
int currentThrottle = minThrottle;

unsigned long ledPreviousMillis = 0; // Переменная для управления миганием светодиода

bool ledState = LOW; // начальное состояние светодиода

bool testRunning = false;
unsigned long testStartTime = 0;
const unsigned long accelTime = 5000; // Время ускорения
const unsigned long holdTime = 8000;   // Время удержания
const unsigned long decelTime = 3000;  // Время замедления

// Переменные для накопления тока в период удержания
float holdTimeCurrentSum = 0.0;
int holdTimeCurrentCount = 0;
bool isHoldTimeActive = false;

void setup() {
  //digitalWrite(resultLedPins[2], HIGH);
  //digitalWrite(resultLedPins[1], HIGH);
  //digitalWrite(resultLedPins[0], HIGH);
  Serial.begin(115200);
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);

  pinMode(BUTTON_PIN_1, INPUT_PULLUP);
  pinMode(BUTTON_PIN_2, INPUT_PULLUP);

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Система запущена. Нажмите кнопку для запуска теста или выбора KV.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Мигающий светодиод, если rpm <= 100 и тест не завершен
  if (rpm <= 100 && !testEnds) {
    if (currentMillis - ledPreviousMillis >= 500) { // Используем ledPreviousMillis только для светодиода
      ledPreviousMillis = currentMillis;
      ledState = !ledState; // Инвертируем состояние
      digitalWrite(resultLedPins[0], ledState); // Записываем новое состояние
    }
  } else if (rpm > 100 && !testEnds) {
    // Если rpm > 100 или тест завершен, выключаем светодиод
    digitalWrite(resultLedPins[0], LOW);
    ledState = LOW; // Сбрасываем состояние
  }

  // Нажатие кнопки для выбора KV
  if (digitalRead(BUTTON_PIN_2) == LOW) {
    delay(400); // Задержка для антидребезга
    if (!testRunning) { // Переключение только если тест не запущен
      selectedKVled = (selectedKVled + 1) % 6;
      selectedKV = KV[selectedKVled];
      for (int i = 0; i < 6; i++) {
        digitalWrite(ledPins[i], i == selectedKVled ? HIGH : LOW);
      }
      Serial.print("Текущее значение KV: ");
      Serial.println(selectedKV);
    }
  }

  // Нажатие кнопки для запуска или остановки теста
  if (digitalRead(BUTTON_PIN_1) == LOW) {
    delay(500); // Задержка для антидребезга

    maxKV=0;

    digitalWrite(resultLedPins[2], LOW);
    digitalWrite(resultLedPins[1], LOW);
    digitalWrite(resultLedPins[0], LOW);
    
    if (!testRunning) {
      // Запуск теста
      testRunning = true;
      testEnds = false;
      testStartTime = currentMillis;

      holdTimeCurrentSum = 0.0;
      holdTimeCurrentCount = 0;
      isHoldTimeActive = false;

      Serial.println("Тест запущен.");
    } else {
      // Остановка теста
      testRunning = false;
      Serial.println("Тест прерван пользователем.");
    }
  }

  if (testRunning) {
    unsigned long elapsedTime = currentMillis - testStartTime;
    //Serial.println(rpm);

    if (elapsedTime < accelTime) {
      currentThrottle = map(elapsedTime, 0, accelTime, minThrottle, maxThrottle);
    } else if (elapsedTime < accelTime + holdTime) {
      currentThrottle = maxThrottle;
      isHoldTimeActive = true;
    } else if (elapsedTime < accelTime + holdTime + decelTime) {
      currentThrottle = map(elapsedTime, accelTime + holdTime, accelTime + holdTime + decelTime, maxThrottle, minThrottle);
      isHoldTimeActive = false;
    } else {
      testRunning = false;
      if (holdTimeCurrentCount > 0) {
        maxKV_AverageCurrent = holdTimeCurrentSum / holdTimeCurrentCount;
      }
      testEnds = true;
      float difference = abs((maxKV - selectedKV) / selectedKV) * 100;
      Serial.println(maxKV);
      Serial.println(selectedKV);
      Serial.println(difference);
      if (difference <= 2) {
          digitalWrite(resultLedPins[2], HIGH);
          Serial.println("Разница менее 2%");
      } else if (difference <= 5) {
          digitalWrite(resultLedPins[1], HIGH);
          Serial.println("Разница менее 5%");
      } else {
          digitalWrite(resultLedPins[0], HIGH);
          Serial.println("Разница более 5%");
      }
      Serial.println("Тест завершен.");
    }
  } else {
    if (currentThrottle > minThrottle) {
      currentThrottle -= 50;
      delay(10);
    }
  }

  esc.writeMicroseconds(currentThrottle);
  //Serial.println(currentThrottle);

  // Измерения
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float revolutions = pulses / float(MAGNET_COUNT);
    rpm = revolutions * 60 *0.9885;

    int sensorValue = analogRead(VOLTAGE_PIN);
    float measuredVoltage = sensorValue * (REFERENCE_VOLTAGE / 962.0);
    voltage = measuredVoltage * DIVIDER_RATIO;

    int currentSensorValue = analogRead(CURRENT_PIN);
    float voltageAtCurrentPin = currentSensorValue * (REFERENCE_VOLTAGE / 1023.0);
    current = (voltageAtCurrentPin - 2.5) / 0.066;

    totalCurrent -= currentReadings[currentReadingIndex];
    currentReadings[currentReadingIndex] = current;
    totalCurrent += current;
    currentReadingIndex = (currentReadingIndex + 1) % numReadings;
    averageCurrent = totalCurrent / numReadings;

    if (isHoldTimeActive) {
      holdTimeCurrentSum += averageCurrent;
      holdTimeCurrentCount++;

      if (voltage != 0) {
        float kv = rpm / voltage;
        if (kv > maxKV) {
          maxKV = kv;
        }
      }
    }
  }

    // Serial.print("Speed: ");
    // //Serial.print(Speed);
    // Serial.print("\t");
    Serial.print("RPM: ");
    Serial.print(rpm);
    Serial.print("\t");
    Serial.print("Current: ");
    Serial.print(current);
    Serial.print("\t");
    Serial.print("Voltage: ");
    Serial.println(voltage);
    delay(100);
}

void countPulse() {
  pulseCount++;
}