#include <Wire.h>
#include "HX711.h"
#include <Servo.h>
#include <avr/sleep.h>

HX711 scale1;
HX711 scale2;
HX711 scale3;

#include <Adafruit_MCP4725.h>

// Создаем экземпляр ЦАП
Adafruit_MCP4725 dac;

// Адрес ЦАП (по умолчанию 0x60, может быть 0x61)
#define DAC_ADDRESS 0x60

float Moment = 0;

#define HALL_PIN_D0 2
#define MAGNET_COUNT 7

// Добавляем изменяемую переменную
volatile int PULSE_THRESHOLD = 70;
volatile double MOMENT_TENZ = 1.0;
volatile double THRUST_TENZ = 1.0;

// Количество оборотов, соответствующих PULSE_THRESHOLD
#define REVS_PER_PULSE_THRESHOLD 10

// Время для сброса (в миллисекундах)
const unsigned long TIME_THRESHOLD = 1000; // 0.8 секунды

volatile int pulseCount = 0; // Переменная для подсчёта импульсов
unsigned long lastTime = 0; // Время последнего считывания
unsigned long startTime = 0; // Время начала накопления импульсов

unsigned long previousMillis = 0;
const long interval = 100; // Интервал для обновления всех данных (0.1 сек)
float rpm = 0;
bool freezeMode = false;  // Флаг для режима FREEZE

int minThrottle = 0;
int maxThrottle = 4095;
int currentSpeed = minThrottle;

bool testInProgress = false;
bool stopCommand = false;
bool holdActive = false;

unsigned long speedUpdateMillis = 0;
bool accelerating = false;
bool decelerating = false;
int nextSpeedThreshold = minThrottle + 100; // Следующий порог скорости для удержания

volatile bool rpmUpdated = false; // Флаг обновления RPM

void setup() {
  Serial.begin(115200);
  Serial.println("Start config...");

  scale1.begin(3, 4);
  scale1.set_scale(260);
  scale1.tare();

  scale2.begin(5, 6);
  scale2.set_scale(260);
  scale2.tare();

  Serial.println("Tenz configurated");

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  Serial.println("Hall configurated");

    dac.begin(DAC_ADDRESS);      // Инициализация ЦАП
    dac.setVoltage(0, false);
  
  Serial.println("System Ready. Use START to begin test or STOP for emergency stop.");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Проверка команд через Serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Удаляем пробелы и символы переноса строки

    if (command == "START_FREEZE" && !testInProgress && !freezeMode) {
      startFreeze();
    } 
    else if (command.startsWith("START") && !testInProgress && !freezeMode) {
      rpm = 0;  // Сбрасываем количество импульсов и время
      pulseCount = 0;
      startTime = 0; // Сбрасываем время начала накопления

      String percentStr = command.substring(6); // Получаем строку после "START_"
      int percent = percentStr.toInt(); // Преобразуем строку в число
      
      if (percent >= 10 && percent <= 100) {
        maxThrottle = map(percent, 10, 100, 410, 4095); // Перевод процента в значение maxThrottle
        startTest();
      } else {
        Serial.println("Ошибка: процент должен быть от 10 до 100.");
      }

    } 
    else if (command == "STOP") {
      stopCommand = true;
    } 
    // Добавляем обработку команды INFO
    else if (command == "INFO") {
      sendStandInfo();  // Вызов функции отправки информации о стенде
    }
    // Добавляем обработку команды PULSE_THRESHOLD_значение
    else if (command.startsWith("PULSE_THRESHOLD_")) { // Добавлено
      String valueStr = command.substring(strlen("PULSE_THRESHOLD_")); // Добавлено
      int newThreshold = valueStr.toInt(); // Добавлено
      
      if (newThreshold > 0) { // Добавлено
        noInterrupts(); // Отключаем прерывания для безопасного изменения переменной
        PULSE_THRESHOLD = newThreshold; // Добавлено
        interrupts(); // Включаем прерывания
        Serial.print("PULSE_THRESHOLD set to: ");
        Serial.println(PULSE_THRESHOLD); // Добавлено
      } else { // Добавлено
        Serial.println("Invalid PULSE_THRESHOLD value."); // Добавлено
      } // Добавлено
    } 
    // Добавляем обработку команды MOMENT_TENZ_значение
    else if (command.startsWith("MOMENT_TENZ_")) { // Добавлено
      String valueStr = command.substring(strlen("MOMENT_TENZ_")); // Добавлено
      double newMomentTenz = valueStr.toDouble(); // Добавлено
      
      if (newMomentTenz > 0) { // Добавлено
        noInterrupts(); // Отключаем прерывания для безопасного изменения переменной
        MOMENT_TENZ = newMomentTenz; // Добавлено
        interrupts(); // Включаем прерывания
        Serial.print("MOMENT_TENZ set to: ");
        Serial.println(MOMENT_TENZ, 4); // Добавлено
      } else { // Добавлено
        Serial.println("Invalid MOMENT_TENZ value."); // Добавлено
      } // Добавлено
    } 
    // Добавляем обработку команды THRUST_TENZ_значение
    else if (command.startsWith("THRUST_TENZ_")) { // Добавлено
      String valueStr = command.substring(strlen("THRUST_TENZ_")); // Добавлено
      double newThrustTenz = valueStr.toDouble(); // Добавлено
      
      if (newThrustTenz > 0) { // Добавлено
        noInterrupts(); // Отключаем прерывания для безопасного изменения переменной
        THRUST_TENZ = newThrustTenz; // Добавлено
        interrupts(); // Включаем прерывания
        Serial.print("THRUST_TENZ set to: ");
        Serial.println(THRUST_TENZ, 4); // Добавлено
      } else { // Добавлено
        Serial.println("Invalid THRUST_TENZ value."); // Добавлено
      } // Добавлено
    } // Добавлено
  }
  
  // Обработка обновления RPM и сбор данных
  noInterrupts(); // Защита доступа к общим переменным
  bool shouldUpdate = rpmUpdated;
  if (shouldUpdate) {
    rpmUpdated = false;
  }
  interrupts();
  
  if (shouldUpdate && testInProgress) {
    // Вычисление RPM уже произошло в ISR, теперь собираем данные
    processSensorData();
  }

  // Если тест в процессе выполнения
  if (testInProgress) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      // Управление ускорением, поддержанием и торможением
      if (accelerating) {
        if (currentSpeed < nextSpeedThreshold) {
          currentSpeed += 40;
          if (currentSpeed > nextSpeedThreshold) currentSpeed = nextSpeedThreshold;
          setSpeed(currentSpeed, decelerating);
        } else {
          holdActive = true;
          accelerating = false;
          speedUpdateMillis = currentMillis;
        }
      }

      if (holdActive) {
        if (currentMillis - speedUpdateMillis >= 1000) { // Удержание
          holdActive = false;
          if (nextSpeedThreshold < maxThrottle) {
            nextSpeedThreshold += 410;
            accelerating = true;
          } else {
            decelerating = true;
          }
        }
      }

      if (decelerating) {
        if (currentSpeed > minThrottle) {
          currentSpeed -= 120;
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

// Функция для обработки данных с датчиков при обновлении RPM
void processSensorData() {
  float Moment = (((scale1.get_units() * 0.001) * 1.1983 - 0.0062) * 0.88)*0.947 * MOMENT_TENZ;
  if (Moment < 0.0){
    Moment = 0;
  }

  float tyaga = scale2.get_units() * THRUST_TENZ;
  if (tyaga < 80) tyaga = 0;

  if (currentSpeed % 100 == 0 && !freezeMode){
    Serial.print("Скорость:");
    Serial.print(currentSpeed);
    Serial.print(":Момент:");
    Serial.print(Moment, 4);
    Serial.print(":Тяга:");
    Serial.print(tyaga, 0);
    Serial.print(":Об/мин:");
    Serial.print(rpm, 0);
    Serial.print("\n");
  }

}

void sendStandInfo() {
  Serial.println("Наименование стенда: шпиндель");
  Serial.println("Измеряет показатели момента, тяги и оборотов в минуту.");
}

// Прерывание для подсчёта импульсов с датчика Холла
void countPulse() {
  // Если это первый импульс, запоминаем время начала накопления
  if (pulseCount == 0) {
    startTime = millis();
  }

  pulseCount++;

  // Если достигнут порог импульсов
  if (pulseCount >= PULSE_THRESHOLD) { // Изменение
    // Расчет времени, когда были получены эти импульсы
    unsigned long currentTime = millis();
    unsigned long timeForPulses = currentTime - startTime;

    // Расчет оборотов в минуту
    if (timeForPulses > 0) {
      // RPM = (количество оборотов * 60) / (время в секундах)
      rpm = (REVS_PER_PULSE_THRESHOLD * 60.0) / (timeForPulses / 1000.0);
    }

    // Сбрасываем количество импульсов и время
    pulseCount = 0;
    startTime = 0; // Сбрасываем время начала накопления

    // Устанавливаем флаг обновления RPM
    rpmUpdated = true;
  }
}

// Функция для плавного разгона двигателя до 20% при команде START_FREEZE
void startFreeze() {
  Serial.println("Entering freeze mode...");
  freezeMode = true;
  currentSpeed = minThrottle; // Стартовая скорость
  int targetSpeed = 820; // 20% мощности
  while (currentSpeed < targetSpeed) {
    currentSpeed += 40;
    setSpeed(currentSpeed, false);
    delay(100);  // Задержка для плавного разгона
  }
  Serial.println("Motor is running at 20% power.");
}

// Поддержание постоянной скорости в режиме FREEZE
void maintainFreezeSpeed() {
  setSpeed(820, false);  // Поддержание скорости 1200 (20%)
}

void setSpeed(int speed, bool deceleratingFlag) {
  dac.setVoltage(speed, false);
  if (speed % 410 == 0 && !freezeMode) {
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
  nextSpeedThreshold = minThrottle + 410;
  speedUpdateMillis = millis();
}

// Объединённая функция для экстренной остановки и выхода из режима FREEZE
void emergencyStop() {
  Serial.println("Stopping motor...");
  
  while (currentSpeed > minThrottle) {
    currentSpeed -= 120;
    if (currentSpeed < minThrottle) currentSpeed = minThrottle;
    setSpeed(currentSpeed, decelerating);
    delay(100);
  }

  testInProgress = false;
  freezeMode = false;  // Отключаем режим заморозки, если он был активен
  stopCommand = false;
  Serial.println("Motor stopped.");
}