#include <Servo.h>
#include <avr/sleep.h>
#include <HX711.h>

// Определяем пины для тензодатчика
#define DOUT  2  // Подключаем к пину D2 на Arduino
#define CLK   3  // Подключаем к пину D3 на Arduino

// Создаем объект HX711
HX711 scale;

// Определяем пины для серво
Servo esc;
int escPin = A2;
int minThrottle = 1000;
int maxThrottle = 2000;
int currentSpeed = minThrottle;

bool testComplete = false;

// Структура для хранения данных
struct LogData {
  unsigned long timestamp;
  int speed;
  long weight; // Добавляем вес
};

LogData logData;

void setup() {
  Serial.begin(9600);

  // Инициализация серво
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);

  // Инициализация тензодатчика
  scale.begin(DOUT, CLK);
  scale.set_scale(387.0); // Установите масштаб вручную в зависимости от вашей калибровки
  scale.tare(); // Устанавливаем текущее значение как нулевое

  Serial.println("System Ready. Use 1-9 to control speed, - to stop and enter sleep mode.");
}

void loop() {
  // Проверяем наличие данных из тензодатчика
  if (scale.is_ready()) {
    logData.weight = scale.get_units(10); // Получаем усредненное значение (10 измерений)
  } else {
    logData.weight = 0; // Если тензодатчик не готов, устанавливаем вес в 0
  }

  // Отправляем данные через последовательный порт каждую половину секунды
  static unsigned long lastSendTime = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= 500) {
    lastSendTime = currentMillis;

    logData.timestamp = millis();
    logData.speed = currentSpeed;
    logDataToSerial(logData);
  }

  // Обработка команд
  while (Serial.available() > 0) {
    char command = Serial.read();
    processCommand(command);
  }

  // Если тест завершен, отправляем окончательные данные и переходим в режим сна
  if (testComplete) {
    // Отправляем конечное время, скорость и вес перед завершением
    logData.timestamp = millis();
    logData.speed = currentSpeed;
    logData.weight = scale.get_units(10); // Финальное значение веса
    logDataToSerial(logData);

    Serial.println("Test complete. Entering sleep mode...");
    enterSleepMode();
  }
}

void processCommand(char command) {
  if (command >= '1' && command <= '9') {
    int targetSpeed = minThrottle + (command - '1' + 1) * (maxThrottle - minThrottle) / 10;
    adjustSpeed(targetSpeed);
  } else if (command == '-') {
    adjustSpeed(minThrottle);
    testComplete = true;
  } else {
    Serial.println("Unknown command.");
  }
}

void adjustSpeed(int targetSpeed) {
  int increment = (targetSpeed > currentSpeed) ? 10 : -10;
  while (currentSpeed != targetSpeed) {
    currentSpeed += increment;
    if ((increment > 0 && currentSpeed > targetSpeed) || (increment < 0 && currentSpeed < targetSpeed)) {
      currentSpeed = targetSpeed;
    }
    setSpeed(currentSpeed);
    delay(100);
  }
}

void setSpeed(int speed) {
  esc.writeMicroseconds(speed);
}

void logDataToSerial(const LogData& data) {
  Serial.print("timestamp,");
  Serial.print(data.timestamp);
  Serial.print(",speed,");
  Serial.print(data.speed);
  Serial.print(",weight,");
  Serial.println(data.weight);
}

void enterSleepMode() {
  Serial.flush();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  // Выйти из режима сна можно только с помощью сброса
}