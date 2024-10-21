#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define HALL_PIN_D0 2 
#define VOLTAGE_PIN A1
#define CURRENT_PIN A2
#define BUTTON_PIN 3

#define MAGNET_COUNT 7
const float REFERENCE_VOLTAGE = 5.0;
const float MAX_INPUT_VOLTAGE = 25.0;
const float DIVIDER_RATIO = MAX_INPUT_VOLTAGE / REFERENCE_VOLTAGE;

const int ledPins[6] = {4, 5, 6, 7, 8, 9}; // Светодиоды для выбора KV
const int resultLedPins[3] = {10, 11, 12}; // Светодиоды для результата
const int KV[6] = {300, 600, 900, 1200, 1500, 1800};
int selectedKVled = 0;
int selectedKV = 300;

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

bool testRunning = false;
unsigned long testStartTime = 0;
const unsigned long accelTime = 10000; // Время ускорения
const unsigned long holdTime = 8000;   // Время удержания
const unsigned long decelTime = 5000;  // Время замедления

// Переменные для накопления тока в период удержания
float holdTimeCurrentSum = 0.0;
int holdTimeCurrentCount = 0;
bool isHoldTimeActive = false;

// Переменные для управления кнопкой
unsigned long buttonPressStart = 0;
const unsigned long longPressTime = 2000; // Время длительного нажатия

void setup() {
  Serial.begin(9600);
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Система запущена. Нажмите и удерживайте кнопку для начала теста.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Проверка кнопки
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (buttonPressStart == 0) {
      buttonPressStart = currentMillis; // Запоминаем время нажатия
    } else if (currentMillis - buttonPressStart >= longPressTime) {
      // Долгое нажатие
      if (!testRunning) {
        // Запуск теста
        testRunning = true;
        testStartTime = currentMillis;

        // Сброс переменных удержания
        holdTimeCurrentSum = 0.0;
        holdTimeCurrentCount = 0;
        isHoldTimeActive = false;

        Serial.println("Тест запущен.");
      }
    }
  } else {
    if (buttonPressStart != 0) {
      // Кнопка отпущена
      unsigned long pressDuration = currentMillis - buttonPressStart;
      buttonPressStart = 0; // Сбрасываем время нажатия

      if (pressDuration < longPressTime) {
        // Короткое нажатие — переключение KV
        if (testRunning){
            Serial.println("Тест остановлен.");
            testRunning = false; // Остановка теста
        } else{
            Serial.println("Переключение KV.");
            selectedKVled = (selectedKVled + 1) % 6;
            selectedKV = KV[selectedKVled];
            for (int i = 0; i < 6; i++) {
            digitalWrite(ledPins[i], i == selectedKVled ? HIGH : LOW);
            Serial.print("Текущее значение KV: ");
            Serial.println(selectedKV);
        }

        }
        
      }
    }
  }

  if (testRunning) {
    unsigned long elapsedTime = currentMillis - testStartTime;

    if (elapsedTime < accelTime) {
      // Ускорение
      currentThrottle = map(elapsedTime, 0, accelTime, minThrottle, maxThrottle);
    } else if (elapsedTime < accelTime + holdTime) {
      // Удержание максимальной скорости
      currentThrottle = maxThrottle;

      // Включаем флаг удержания
      isHoldTimeActive = true;

    } else if (elapsedTime < accelTime + holdTime + decelTime) {
      // Замедление
      currentThrottle = map(elapsedTime, accelTime + holdTime, accelTime + holdTime + decelTime, maxThrottle, minThrottle);

      // Фаза удержания завершена
      isHoldTimeActive = false;

    } else {
      // Завершение теста
      testRunning = false;

      // Выводим средний ток за период удержания
      if (holdTimeCurrentCount > 0) {
        maxKV_AverageCurrent = holdTimeCurrentSum / holdTimeCurrentCount;
      }
        // Сравнение maxKV с выбранным KV
        float difference = abs((maxKV - selectedKV) / selectedKV) * 100;

        if (difference <= 5) {
            digitalWrite(resultLedPins[0], HIGH); // Менее 5%
            Serial.println("Разница менее 5%");
        } else if (difference <= 10) {
            digitalWrite(resultLedPins[1], HIGH); // Менее 10%
            Serial.println("Разница менее 10%");
        } else {
            digitalWrite(resultLedPins[2], HIGH); // Более 10%
            Serial.println("Разница более 10%");
        }

      Serial.println("Тест завершен.");
    }
  } else {
    // Остановка двигателя, если тест не запущен
    if (currentThrottle > minThrottle) {
      currentThrottle -= 10;
      delay(10);
    }
  }

  // Код для управления ESC
  esc.writeMicroseconds(currentThrottle);
  Serial.println(currentThrottle);
  Serial.println(pulseCount);

  // Код для измерений и дисплея
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float revolutions = pulses / float(MAGNET_COUNT);
    rpm = revolutions * 60 * 0.9859;

    int sensorValue = analogRead(VOLTAGE_PIN);
    float measuredVoltage = sensorValue * (REFERENCE_VOLTAGE / 1020.0);
    voltage = measuredVoltage * DIVIDER_RATIO;

    int currentSensorValue = analogRead(CURRENT_PIN);
    float voltageAtCurrentPin = currentSensorValue * (REFERENCE_VOLTAGE / 1023.0);
    current = (voltageAtCurrentPin - 2.5) / 0.066;

    // Считаем средний ток
    totalCurrent -= currentReadings[currentReadingIndex];
    currentReadings[currentReadingIndex] = current;
    totalCurrent += current;
    currentReadingIndex = (currentReadingIndex + 1) % numReadings;
    averageCurrent = totalCurrent / numReadings;

    // Накопление данных тока в фазе удержания
    if (isHoldTimeActive) {
      holdTimeCurrentSum += averageCurrent;
      holdTimeCurrentCount++;

      // Обновление maxKV в период удержания
      if (voltage != 0) {
        float kv = rpm / voltage;
        if (kv > maxKV) {
          maxKV = kv;
        }
      }
    }
  }

Serial.print("RPM: ");
Serial.println(rpm);
Serial.print("V: ");
Serial.println(voltage);
Serial.print("Avg. A: ");
Serial.println(averageCurrent * -1);

Serial.print("KVmax:");
Serial.println(maxKV, 0);
Serial.print("Akv:");
Serial.println(maxKV_AverageCurrent * -1);

}

void countPulse() {
  pulseCount++;
}