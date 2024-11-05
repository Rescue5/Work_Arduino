#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define HALL_PIN_D0 2 
#define VOLTAGE_PIN A5
#define CURRENT_PIN A6
#define BUTTON_PIN1 A2 // Первая кнопка для нажатий
#define BUTTON_PIN2 3 // Вторая кнопка для удержания
#define MAGNET_COUNT 7
const float REFERENCE_VOLTAGE = 5.0;
const float MAX_INPUT_VOLTAGE = 25.0;
const float DIVIDER_RATIO = MAX_INPUT_VOLTAGE / REFERENCE_VOLTAGE;

const int ledPins[6] = {12, 11, 10, 9, 8, 7}; // Светодиоды для выбора KV
const int resultLedPins[3] = {4, 5, 6}; // Светодиоды для результата
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
int escPin = A7;
int minThrottle = 1000;
int maxThrottle = 1500;
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

// Переменные для управления кнопками
unsigned long buttonPressStart1 = 0;
unsigned long buttonPressStart2 = 0;
const unsigned long longPressTime = 1000; // Время длительного нажатия для удержания второй кнопкой
unsigned long lastButtonStateChange1 = 0;
unsigned long lastButtonStateChange2 = 0;
const unsigned long debounceDelay = 200; // Задержка для антидребезга
bool isButtonLongPress1 = false;
bool isButtonLongPress2 = false;

void setup() {
  Serial.begin(115200);
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);

  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Система запущена. Нажмите первую кнопку для переключения KV, вторую кнопку для запуска теста.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Проверка первой кнопки (нажатие)
  if (digitalRead(BUTTON_PIN1) == LOW) {
    if (buttonPressStart1 == 0) {
      buttonPressStart1 = currentMillis; // Запоминаем время нажатия
    } else if (currentMillis - buttonPressStart1 >= longPressTime) {
      // Долгое нажатие
      isButtonLongPress1 = true;
    }
  } else {
    if (buttonPressStart1 != 0) {
      unsigned long pressDuration = currentMillis - buttonPressStart1;
      buttonPressStart1 = 0;

      if (isButtonLongPress1) {
        isButtonLongPress1 = false; // Сбрасываем флаг долгого нажатия
      } else if (pressDuration < longPressTime && (currentMillis - lastButtonStateChange1 > debounceDelay)) {
        // Короткое нажатие — переключение KV
        if (!testRunning) {  // Разрешаем переключение только если тест не запущен
          Serial.println("Переключение KV.");
          selectedKVled = (selectedKVled + 1) % 6;
          selectedKV = KV[selectedKVled];
          for (int i = 0; i < 6; i++) {
            digitalWrite(ledPins[i], i == selectedKVled ? HIGH : LOW);
          }
          Serial.print("Текущее значение KV: ");
          Serial.println(selectedKV);
        }
        lastButtonStateChange1 = currentMillis; // Обновляем время последнего изменения состояния кнопки
      }
    }
  }

  // Проверка второй кнопки (удержание)
  if (digitalRead(BUTTON_PIN2) == LOW) {
    if (buttonPressStart2 == 0) {
      buttonPressStart2 = currentMillis; // Запоминаем время нажатия второй кнопки
    } else if (currentMillis - buttonPressStart2 >= longPressTime) {
      // Долгое нажатие второй кнопки запускает тест
      if (!testRunning) {
        testRunning = true;


testStartTime = currentMillis;

        // Сброс переменных удержания
        holdTimeCurrentSum = 0.0;
        holdTimeCurrentCount = 0;
        isHoldTimeActive = false;

        Serial.println("Тест запущен.");
      }
      isButtonLongPress2 = true;
    }
  } else {
    if (buttonPressStart2 != 0) {
      buttonPressStart2 = 0;
      if (isButtonLongPress2) {
        isButtonLongPress2 = false;
      }
    }
  }

  // Код для проведения теста
  if (testRunning) {
    unsigned long elapsedTime = currentMillis - testStartTime;

    if (elapsedTime < accelTime) {
      // Ускорение
      currentThrottle = map(elapsedTime, 0, accelTime, minThrottle, maxThrottle);
    } else if (elapsedTime < accelTime + holdTime) {
      // Удержание максимальной скорости
      currentThrottle = maxThrottle;
      isHoldTimeActive = true;
    } else if (elapsedTime < accelTime + holdTime + decelTime) {
      // Замедление
      currentThrottle = map(elapsedTime, accelTime + holdTime, accelTime + holdTime + decelTime, maxThrottle, minThrottle);
      isHoldTimeActive = false;
    } else {
      // Завершение теста
      testRunning = false;
      if (holdTimeCurrentCount > 0) {
        maxKV_AverageCurrent = holdTimeCurrentSum / holdTimeCurrentCount;
      }
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
    if (currentThrottle > minThrottle) {
      currentThrottle -= 10;
      delay(10);
    }
  }

  // Управление ESC
  esc.writeMicroseconds(currentThrottle);

  // Измерения и вывод данных
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

    // Среднее значение тока
    totalCurrent -= currentReadings[currentReadingIndex];
    currentReadings[currentReadingIndex] = current;
    totalCurrent += current;
    currentReadingIndex = (currentReadingIndex + 1) % numReadings;
    averageCurrent = totalCurrent / numReadings;

    // Накопление данных тока в фазе удержания
    if (isHoldTimeActive) {
      holdTimeCurrentSum += averageCurrent;
      holdTimeCurrentCount++;

      // Обновление maxKV
      if (voltage != 0) {
        float kv = rpm / voltage;
        if (kv > maxKV) {
          maxKV = kv;
        }
      }
    }

    Serial.print("RPM: ");
    Serial.print(rpm);
    Serial.print("\t");
    Serial.print("Current: ");
    Serial.println(current);
  }
}

void countPulse() {
  pulseCount++;
}