#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

// Определите размер дисплея и его адрес
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

HX711 scale1;
HX711 scale2;
HX711 scale3;

const int numReadings = 10;  // Количество измерений для усреднения

float readings1[numReadings];  // Массив для хранения измерений первого тензодатчика
float readings2[numReadings];  // Массив для хранения измерений второго тензодатчика
float Moment1 = 0;
float Moment2 = 0;

int readIndex = 0;  // Индекс текущего чтения
float total1 = 0;   // Сумма измерений для первого тензодатчика
float total2 = 0;   // Сумма измерений для второго тензодатчика

// Пины для датчика Холла
#define HALL_PIN_D0 2

// Количество магнитов на диске
#define MAGNET_COUNT 2

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 1000;  // Интервал измерений в миллисекундах
float rpm = 0;

void setup() {
  Serial.begin(115200);

  

  // Инициализация первого тензодатчика
  scale1.begin(3, 4);
  scale1.set_scale(260);  // Замените на ваш коэффициент калибровки для кг
  scale1.tare();          // Обнуление тары

  // Инициализация второго тензодатчика
  scale2.begin(5, 6);
  scale2.set_scale(260);  // Замените на ваш коэффициент калибровки для кг
  scale2.tare();

  scale3.begin(9, 10);
  scale3.set_scale(218);  // Замените на ваш коэффициент калибровки для кг
  scale3.tare();

  // Инициализация массивов
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings1[thisReading] = 0;
    readings2[thisReading] = 0;
  }

  // Инициализация датчика Холла
  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

 
}

void loop() {
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
  // if (average1 < 0.080) average1 = 0;
  // if (average2 < 0.080) average2 = 0;

  // Расчет момента на валу
  Moment1 = 2.0566 * average1;
  Moment2 = 2.0566 * average2;
  // if (Moment < 0.0050) Moment = 0;
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
    rpm = revolutions * 60;                            // Переводим в обороты за минуту
  }

  float thrust = scale3.get_units() * 0.001;

  // Вывод усредненных значений на консоль
  //if (average1 > 0.020 || Moment > 0.010 || (rpm > 300 && rpm < 25000 && average2 > 0.080)) {
    Serial.print("Момент1: ");
    Serial.print(average1, 4);
    Serial.print(" кг ");
    Serial.print(Moment1, 4);
    Serial.print(" Н*м ");
    Serial.print("     ");
    Serial.print("Момент2: ");
    Serial.print(average2, 4);
    Serial.print(" кг ");
    Serial.print(Moment2, 4);
    Serial.print(" Н*м ");
    Serial.print("    ");
    Serial.print("Тяга: ");
    Serial.print(thrust, 4);
    Serial.print(" кг ");
    Serial.print("Об/мин: ");
    Serial.print(rpm, 4);
    Serial.println();
  //}
 

  delay(50);  // Задержка 0.02 секунды перед следующей итерацией
}

void countPulse() {
  pulseCount++;
}
