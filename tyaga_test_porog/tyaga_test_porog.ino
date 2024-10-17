#include <Wire.h>
#include "HX711.h"
#include <Servo.h>
#include <avr/sleep.h>

HX711 scale1;
HX711 scale2;

float Moment = 0;

#define HALL_PIN_D0 2
#define MAGNET_COUNT 7

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 100; // Интервал для обновления всех данных (0.1 сек)
float rpm = 0;
bool freezeMode = false;  // Флаг для режима FREEZE

Servo esc;
int escPin = A1;
int minThrottle = 1000;
int maxThrottle = 1300;
int currentSpeed = minThrottle;

bool testInProgress = false;
bool stopCommand = false;
bool holdActive = false;

unsigned long speedUpdateMillis = 0;
bool accelerating = false;
bool decelerating = false;
int nextSpeedThreshold = minThrottle + 100; // Следующий порог скорости для удержания

void setup() {
  Serial.begin(115200);
  Serial.println("Start config...");

  scale1.begin(3, 4);
  scale1.set_scale(850);
  scale1.tare();
  scale2.begin(5, 6);
  scale2.set_scale(109);
  scale2.tare();

  Serial.println("Tenz configurated");

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Hall configurated");

  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);
  
  Serial.println("System Ready. Use START to begin test or STOP for emergency stop.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Проверка команд через Serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');

    if (command == "START_FREEZE" && !testInProgress && !freezeMode) {
      startFreeze();
    } else if (command.startsWith("START") && !testInProgress && !freezeMode) {
      String percentStr = command.substring(6); // Получаем строку после "START_"
      int percent = percentStr.toInt(); // Преобразуем строку в число
      
      if (percent >= 10 && percent <= 100) {
        maxThrottle = map(percent, 10, 100, 1100, 2000); // Перевод процента в значение maxThrottle
        startTest();
      } else {
        Serial.println("Ошибка: процент должен быть от 10 до 100.");
      }

    } else if (command == "STOP") {
      stopCommand = true;
    } 
    // Добавляем обработку команды INFO
    else if (command == "INFO") {
      sendStandInfo();  // Вызов функции отправки информации о стенде
    }
  }

  // Если тест в процессе выполнения
  if (testInProgress) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      // Логирование данных
      logSensors(holdActive, accelerating, decelerating);

      // Управление ускорением, поддержанием и торможением
      if (accelerating) {
        if (currentSpeed < nextSpeedThreshold) {
          currentSpeed += 10;
          if (currentSpeed > nextSpeedThreshold) currentSpeed = nextSpeedThreshold;
          setSpeed(currentSpeed, decelerating);
        } else {
          holdActive = true;
          accelerating = false;
          speedUpdateMillis = currentMillis;
        }
      }

      if (holdActive) {
        if (currentMillis - speedUpdateMillis >= 1000) { // Удержание 1.5 секунды
          holdActive = false;
          if (nextSpeedThreshold < maxThrottle) {
            nextSpeedThreshold += 100;
            accelerating = true;
          } else {
            decelerating = true;
          }
        }
      }

      if (decelerating) {
        if (currentSpeed > minThrottle) {
          currentSpeed -= 30;
          if (currentSpeed < minThrottle) currentSpeed = minThrottle;
          setSpeed(currentSpeed, decelerating);
        } else {
          decelerating = false;
          testInProgress = false;
          Serial.println("Test complete.");
        }
      }
    }

    if (stopCommand) {
      emergencyStop();
    }
  }

  // Если включен режим FREEZE
  if (freezeMode) {
    maintainFreezeSpeed();

    // Проверка на команду STOP во время режима FREEZE
    if (stopCommand) {
      emergencyStop();  // Останавливаем мотор
      freezeMode = false;  // Выключаем режим заморозки
    }
  }
}

// Функция для плавного разгона двигателя до 20% при команде START_FREEZE
void startFreeze() {
  Serial.println("Entering freeze mode...");
  freezeMode = true;
  currentSpeed = minThrottle; // Стартовая скорость
  int targetSpeed = 1200; // 20% мощности
  while (currentSpeed < targetSpeed) {
    currentSpeed += 10;
    setSpeed(currentSpeed, false);
    delay(100);  // Задержка для плавного разгона
  }
  Serial.println("Motor is running at 20% power (1200).");
}

// Поддержание постоянной скорости в режиме FREEZE
void maintainFreezeSpeed() {
  setSpeed(1200, false);  // Поддержание скорости 1200 (20%)
}

void setSpeed(int speed, bool decelerating) {
  esc.writeMicroseconds(speed);
  if (speed % 100 == 0 && !freezeMode) {
    Serial.print("Speed set to: ");
    Serial.println(speed);
  }
}

void startTest() {
  Serial.println("Test started.");
  testInProgress = true;
  stopCommand = false;
  currentSpeed = minThrottle;
  accelerating = true;
  decelerating = false;
  holdActive = false;
  nextSpeedThreshold = minThrottle + 100;
  speedUpdateMillis = millis();
}

// Объединённая функция для экстренной остановки и выхода из режима FREEZE
void emergencyStop() {
  Serial.println("Stopping motor...");
  
  while (currentSpeed > minThrottle) {
    currentSpeed -= 30;
    if (currentSpeed < minThrottle) currentSpeed = minThrottle;
    setSpeed(currentSpeed, decelerating);
    delay(100);
  }

  testInProgress = false;
  freezeMode = false;  // Отключаем режим заморозки, если он был активен
  stopCommand = false;
  Serial.println("Motor stopped.");
}

void logSensors(bool holdActive, bool accelerating, bool decelerating) {
  float Moment_1 = scale1.get_units() * 0.001;
  float Moment_2 = 2.0566 * Moment_1;
  if (Moment_2 < 0.0050) Moment_2 = 0;
  float tyaga = scale2.get_units();
  if (tyaga < 80) tyaga = 0;

  noInterrupts();
  int pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  float revolutions = pulses / float(MAGNET_COUNT);
  rpm = revolutions * 60 * 0.9859 * (1000 / interval);
  if (holdActive && !decelerating && !accelerating) {
    Serial.print("Скорость:");
    Serial.print(currentSpeed);
    Serial.print(":Момент:");
    Serial.print(Moment_1, 4);
    Serial.print(":Тяга:");
    Serial.print(tyaga, 0);
    Serial.print(":Об/мин:");
    Serial.print(rpm, 0);
    Serial.print("\n");
  }
}

void sendStandInfo() {
  Serial.println("Наименование стенда: пропеллер");
  Serial.println("Измеряет показатели тяги и оборотов в минуту.");
}

void countPulse() {
  pulseCount++;
}