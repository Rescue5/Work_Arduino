#include <Servo.h>

// Пины для кнопки и ESC
#define BUTTON_PIN 3
#define ESC_PIN A7

// Переменные для состояния системы
enum TestState { 
  IDLE, 
  ACCELERATING, 
  HOLDING, 
  DECELERATING, 
  ABORTING 
};
TestState currentState = IDLE;

Servo esc; // Создаем объект Servo для ESC
unsigned long lastButtonPress = 0;
unsigned long stateChangeTime = 0;

const int throttleMin = 1000; // Минимальный сигнал для ESC (0%)
const int throttleMax = 2000; // Максимальный сигнал для ESC (100%)
int currentThrottle = throttleMin;

bool buttonPressed = false;

void setup() {
  Serial.begin(9600);

  // Настройка кнопки и ESC
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Используем внутренний подтягивающий резистор
  esc.attach(ESC_PIN);
  esc.writeMicroseconds(throttleMin); // Инициализируем ESC с минимальным значением

  Serial.println("Система готова к работе.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Проверка кнопки
  if (digitalRead(BUTTON_PIN) == LOW && currentMillis - lastButtonPress > 500) {
    buttonPressed = !buttonPressed;
    lastButtonPress = currentMillis;

    if (buttonPressed && currentState == IDLE) {
      currentState = ACCELERATING;
      stateChangeTime = currentMillis;
      Serial.println("Запуск теста...");
    } else if (buttonPressed && currentState != IDLE) {
      currentState = ABORTING;  // Прерывание теста
      Serial.println("Прерывание теста...");
    }
  }

  // Обработка состояний
  switch (currentState) {
    case ACCELERATING:
      Serial.println("ACCELERATING");
      if (currentMillis - stateChangeTime < 10000) {  // Плавное увеличение скорости за 10 секунд
        int elapsedTime = currentMillis - stateChangeTime;
        currentThrottle = map(elapsedTime, 0, 10000, throttleMin, throttleMax);
        esc.writeMicroseconds(currentThrottle);
        Serial.print("Текущая скорость: ");
        Serial.println(currentThrottle);
      } else {
        currentState = HOLDING;
        stateChangeTime = currentMillis;
        Serial.println("Максимальная скорость достигнута. Удержание скорости...");
      }
      break;

    case HOLDING:
      Serial.println("HOLDING");
      if (currentMillis - stateChangeTime < 5000) {  // Держим максимальные обороты в течение 5 секунд
        esc.writeMicroseconds(throttleMax);
      } else {
        currentState = DECELERATING;
        stateChangeTime = currentMillis;
        Serial.println("Снижение оборотов...");
      }
      break;

    case DECELERATING:
      Serial.println("DECELERATING");
      if (currentMillis - stateChangeTime < 10000) {  // Плавное снижение скорости в течение 10 секунд
        int elapsedTime = currentMillis - stateChangeTime;
        currentThrottle = map(elapsedTime, 0, 10000, throttleMax, throttleMin);
        esc.writeMicroseconds(currentThrottle);
        Serial.print("Текущая скорость: ");
        Serial.println(currentThrottle);
      } else {
        currentState = IDLE;
        Serial.println("Тест завершен. Возвращение в состояние IDLE.");
      }
      break;

    case ABORTING:
      Serial.println("ABORTING");
      if (currentThrottle > throttleMin) {
        currentThrottle -= 10;  // Плавное снижение скорости
        esc.writeMicroseconds(currentThrottle);
        Serial.print("Плавное снижение: ");
        Serial.println(currentThrottle);
      } else {
        currentState = IDLE;
        Serial.println("Тест прерван. Возвращение в состояние IDLE.");
      }
      break;

    case IDLE:
      esc.writeMicroseconds(throttleMin);  // Двигатель на холостом ходу
      break;
  }
}