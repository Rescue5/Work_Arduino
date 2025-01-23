#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// Параметры дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Используем -1 если у дисплея нет пина сброса
#define SCREEN_ADDRESS 0x3C  // Адрес I2C дисплея (может быть 0x3D)

// Создаем объект дисплея
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define HALL_PIN_D0 2
#define VOLTAGE_PIN A1
#define CURRENT_PIN A2
#define BUTTON_PIN 3

#define MAGNET_COUNT 7
const float REFERENCE_VOLTAGE = 5.0;
const float MAX_INPUT_VOLTAGE = 25.0;
const float DIVIDER_RATIO = MAX_INPUT_VOLTAGE / REFERENCE_VOLTAGE;

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
const unsigned long accelTime = 5000; // Время ускорения
const unsigned long holdTime = 8000;   // Время удержания
const unsigned long decelTime = 3000;  // Время замедления

// Переменные для накопления тока в период удержания
float holdTimeCurrentSum = 0.0;
int holdTimeCurrentCount = 0;
bool isHoldTimeActive = false;

void setup() {
  Serial.begin(115200);
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);

   //Инициализация дисплея
   if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Ошибка инициализации дисплея SSD1306"));
     for (;;);  // Бесконечный цикл в случае ошибки
   }
   display.clearDisplay();
   display.setTextSize(1);
   display.setTextColor(SSD1306_WHITE);
   display.display();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Система запущена. Нажмите кнопку для начала теста.");
}



void loop() {
  unsigned long currentMillis = millis();

  // Проверка кнопки
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // Дебаунсинг
    maxKV = 0;
    if (digitalRead(BUTTON_PIN) == LOW) {
      while (digitalRead(BUTTON_PIN) == LOW); // Ожидание отпускания кнопки
      if (testRunning) {
        // Остановка теста
        testRunning = false;
        Serial.println("Тест остановлен.");
      } else {
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
      Serial.println("Тест завершен.");
    }
  } else {
    // Остановка двигателя, если тест не запущен
    if (currentThrottle > minThrottle) {
      currentThrottle -= 50;
      delay(10);
    }
  }

  // Код для управления ESC
  esc.writeMicroseconds(currentThrottle);
  Serial.println(currentThrottle);
  //Serial.println(pulseCount);

  // Код для измерений и дисплея
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float revolutions = pulses / float(MAGNET_COUNT);
    rpm = revolutions * 60 * 0.9885 * 0.968;

    int sensorValue = analogRead(VOLTAGE_PIN);
    float measuredVoltage = sensorValue * (REFERENCE_VOLTAGE / 1009.0); //1020
    voltage = measuredVoltage * DIVIDER_RATIO * 0.97;

    int currentSensorValue = analogRead(CURRENT_PIN);
    float voltageAtCurrentPin = currentSensorValue * (REFERENCE_VOLTAGE / 1023.0);
    current = (voltageAtCurrentPin - 2.5) / 0.066 * -1;

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
    // Serial.print("Speed: ");
    // Serial.print(Speed);
    // Serial.print("\t");
    Serial.print("RPM: ");
    Serial.print(rpm);
    Serial.print("\t");
    Serial.print("Current: ");
    Serial.print(current);
    Serial.print("\t");
    Serial.print("Voltage: ");
    Serial.println(voltage);

    // Отображение на дисплее
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("RPM: ");
    display.println(rpm);
    display.print("V: ");
    display.println(voltage);
    display.print("Avg. A: ");
    display.println(averageCurrent * -1);

    display.setTextSize(2);
    display.setCursor(0, 30);
    display.print("KVmax:");
    display.println(maxKV, 0);
    display.print("Akv:");
    display.println(maxKV_AverageCurrent * -1);

    display.display();

   // delay(100);
}

void countPulse() {
  pulseCount++;
}
