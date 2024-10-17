#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"
#include <Servo.h>
#include <avr/sleep.h>

// Определите размер дисплея и его адрес
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

HX711 scale1;
HX711 scale2;

const int numReadings = 10;  // Количество измерений для усреднения

float readings1[numReadings];  // Массив для хранения измерений первого тензодатчика
float readings2[numReadings];  // Массив для хранения измерений второго тензодатчика
float Moment = 0;

int readIndex = 0;  // Индекс текущего чтения
float total1 = 0;   // Сумма измерений для первого тензодатчика
float total2 = 0;   // Сумма измерений для второго тензодатчика

// Пины для датчика Холла
#define HALL_PIN_D0 8

// Количество магнитов на диске
#define MAGNET_COUNT 7

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 1000;  // Интервал измерений в миллисекундах
float rpm = 0;

// Настройки ESC
Servo esc;
int escPin = A1;
int minThrottle = 1000;
int maxThrottle = 2000;
int currentSpeed = minThrottle;

bool testComplete = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Start config...");

  // Инициализация первого тензодатчика
  // scale1.begin(3, 4);
  // scale1.set_scale(850);  // Замените на ваш коэффициент калибровки для кг
  // scale1.tare();          // Обнуление тары
  // Serial.println("Tenzo - 1 config success");
  // delay(500);

  // // Инициализация второго тензодатчика
  // scale2.begin(5, 6);
  // scale2.set_scale(109);  // Замените на ваш коэффициент калибровки для кг
  // scale2.tare();
  // Serial.println("Tenzo - 2 config success");
  // delay(500);

  // Инициализация массивов
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings1[thisReading] = 0;
    readings2[thisReading] = 0;
  }
  Serial.println("Arrays conf success");
  delay(500);

  // Инициализация датчика Холла
  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);
  Serial.println("Hall conf success");
  delay(500);

  // Инициализация ESC
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);
  Serial.println("ESK conf success");
  delay(500);
  
  Serial.println("System Ready. Use 0-9 to control speed and 10 to set 100%, - to stop and enter sleep mode.");
}

void loop() {
  while (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }

  // Удаляем старое значение из суммы
  total1 = total1 - readings1[readIndex];
  total2 = total2 - readings2[readIndex];

  // Чтение нового значения
  readings1[readIndex] = scale1.get_units() * 0.001;  // Переводим из грамм в килограммы
  readings2[readIndex] = scale2.get_units() * 0.001;

  // Добавляем новое значение к сумме
  total1 = total1 + readings1[readIndex];
  total2 = total2 + readings2[readIndex];

  // Переход к следующему индексу
  readIndex = readIndex + 1;

  // Если достигли конца массива, переходим в начало
  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  // Усредняем значения
  float average1 = (total1 / numReadings);
  float average2 = (total2 / numReadings);

  // Коррекция отрицательных значений
  if (average1 < 0.080) average1 = 0;
  if (average2 < 0.080) average2 = 0;

  // Расчет момента на валу
  Moment = 2.0566 * average1;
  if (Moment < 0.0050) Moment = 0;

  // Получаем текущее время
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    // Переводим импульсы в полные обороты в минуту (RPM)
    float revolutions = pulses / float(MAGNET_COUNT);  // Количество полных оборотов за секунду
    rpm = revolutions * 60 * 0.9859; // Переводим в обороты за минуту
  }

  // Вывод усредненных значений на консоль
  //if (average1 > 0.020 || Moment > 0.010 || (rpm > 300 && rpm < 25000 && average2 > 0.080)) {
    Serial.print("Момент: ");
    Serial.print(average1, 4);
    Serial.print(" кг ");
    Serial.print(Moment, 4);
    Serial.print(" Н*м ");
    Serial.print("Тяга: ");
    Serial.print(average2, 4);
    Serial.print(" кг ");
    Serial.print("Об/мин: ");
    Serial.print(rpm, 4);
    Serial.println();
  //}

  if (testComplete) {
    enterSleepMode();
  }
  
  delay(50);  // Задержка 0.05 секунды перед следующей итерацией
}

void processCommand(const String& command) {
  if (command == "-") {
    adjustSpeed(minThrottle);
    Serial.println("Test complete. Entering sleep mode...");
    testComplete = true;
  } else if (command == "10") {
    adjustSpeed(maxThrottle);
  } else if (command.length() == 1 && command >= "0" && command <= "9") {
    int index = command.toInt();
    int targetSpeed = minThrottle + index * (maxThrottle - minThrottle) / 10; // 0-10 соответствует 0%-100%
    adjustSpeed(targetSpeed);
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
  Serial.print("Speed set to: ");
  Serial.println(speed);
}

void enterSleepMode() {
  Serial.flush();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  // Выйти из режима сна можно только с помощью сброса
}

void countPulse() {
  pulseCount++;
}