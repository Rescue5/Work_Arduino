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
float Moment = 0;

int readIndex = 0;  // Индекс текущего чтения
float total1 = 0;   // Сумма измерений для первого тензодатчика
float total2 = 0;   // Сумма измерений для второго тензодатчика

// Пины для датчика Холла
#define HALL_PIN_D0 2

// Количество магнитов на диске
#define MAGNET_COUNT 7

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 1000;  // Интервал измерений в миллисекундах
float rpm = 0;

void setup() {
  Serial.begin(115200);

  

  // Инициализация первого тензодатчика
  scale1.begin(4, 3);
  scale1.set_scale(108);  // Замените на ваш коэффициент калибровки для кг
  scale1.tare();          // Обнуление тары

  // Инициализация второго тензодатчика
  scale2.begin(6, 5);
  scale2.set_scale(105);  // Замените на ваш коэффициент калибровки для кг
  scale2.tare();

  // scale3.begin(7, 8);
  // scale3.set_scale(218);  // Замените на ваш коэффициент калибровки для кг
  // scale3.tare();

  // Инициализация массивов
  // for (int thisReading = 0; thisReading < numReadings; thisReading++) {
  //   readings1[thisReading] = 0;
  //   readings2[thisReading] = 0;
  // }

  // Инициализация датчика Холла
  // pinMode(HALL_PIN_D0, INPUT);
  // attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

 
}

void loop() {
  // Удаляем старое значение из суммы
  // total1 = total1 - readings1[readIndex];
  // total2 = total2 - readings2[readIndex];

  // // Чтение нового значения
  // readings1[readIndex] = scale1.get_units();  // Переводим из грамм в килограммы
  // readings2[readIndex] = scale2.get_units();

  // Добавляем новое значение к сумме
  // total1 = total1 + readings1[readIndex];
  // total2 = total2 + readings2[readIndex];

  // Переход к следующему индексу
  // readIndex = readIndex + 1;

  // Если достигли конца массива, переходим в начало
  // if (readIndex >= numReadings) {
  //   readIndex = 0;
  // }

  // Усредняем значения
  // float average1 = (total1 / numReadings);
  // float average2 = (total2 / numReadings);

  // Коррекция отрицательных значений
  //if (average1 < 0.080) average1 = 0;
  //if (average2 < 0.080) average2 = 0;

  // Расчет момента на валу
  // Moment = 2.0566 * average1;
  // if (Moment < 0.0050) Moment = 0;
  // // Получаем текущее время
  // unsigned long currentMillis = millis();

  // if (currentMillis - previousMillis >= interval) {
  //   previousMillis = currentMillis;
  //   noInterrupts();
  //   int pulses = pulseCount;
  //   pulseCount = 0;
  //   interrupts();

    // Переводим импульсы в полные обороты в минуту (RPM)
    // float revolutions = pulses / float(MAGNET_COUNT);  // Количество полных оборотов за секунду
    // rpm = revolutions * 60*0.9859;                            // Переводим в обороты за минуту
 // }

  // Вывод усредненных значений на консоль
  //if (average1 > 0.020 || Moment > 0.010 || (rpm > 300 && rpm < 25000 && average2 > 0.080)) {
    Serial.print("Тензодатчик 1: ");
    Serial.print(scale1.get_units() * 0.001, 4);
    Serial.print(" ");
    Serial.print("Тензодатчик 2: ");
    Serial.print(scale2.get_units() * 0.001, 4);
    Serial.print(" ");
    // Serial.print("Тензодатчик 3: ");
    // Serial.print(scale3.get_units() * 0.001, 4);
    Serial.print("\n");
  //}
 

  delay(50);  // Задержка 0.02 секунды перед следующей итерацией
}

void countPulse() {
  pulseCount++;
}
