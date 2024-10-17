#include "HX711.h"

// Определяем пины для подключения тензодатчиков
#define DT1 2
#define SCK1 3

#define DT2 4
#define SCK2 5

#define DT3 6
#define SCK3 7

#define DT4 8
#define SCK4 9

#define DT5 10
#define SCK5 11

HX711 scale1;
HX711 scale2;
HX711 scale3;
HX711 scale4;
HX711 scale5;

const int numReadings = 10;  // Количество измерений для усреднения
float readings1[numReadings];  // Массив для хранения данных тензодатчика 1
float readings2[numReadings];  // Массив для хранения данных тензодатчика 2
float readings3[numReadings];  // Массив для хранения данных тензодатчика 3
float readings4[numReadings];  // Массив для хранения данных тензодатчика 4
float readings5[numReadings];  // Массив для хранения данных тензодатчика 5
int readIndex = 0;  // Индекс текущего измерения
float total1 = 0;  // Сумма значений тензодатчика 1
float total2 = 0;  // Сумма значений тензодатчика 2
float total3 = 0;  // Сумма значений тензодатчика 3
float total4 = 0;  // Сумма значений тензодатчика 4
float total5 = 0;  // Сумма значений тензодатчика 5
float average1 = 0;  // Усредненное значение тензодатчика 1
float average2 = 0;  // Усредненное значение тензодатчика 2
float average3 = 0;  // Усредненное значение тензодатчика 3
float average4 = 0;  // Усредненное значение тензодатчика 4
float average5 = 0;  // Усредненное значение тензодатчика 5

void setup() {
  Serial.begin(115200);

  scale1.begin(DT1, SCK1);
  scale2.begin(DT2, SCK2);
  scale3.begin(DT3, SCK3);
  scale4.begin(DT4, SCK4);
  scale5.begin(DT5, SCK5);

  scale1.set_scale(500);
  scale2.set_scale(500);
  scale3.set_scale(450);
  scale4.set_scale(505);
  scale5.set_scale(500);

  scale1.tare();
  scale2.tare();
  scale3.tare();
  scale4.tare();
  scale5.tare();

  // Инициализация массивов
  for (int i = 0; i < numReadings; i++) {
    readings1[i] = 0;
    readings2[i] = 0;
    readings3[i] = 0;
    readings4[i] = 0;
    readings5[i] = 0;
  }
}

void loop() {
  // Убираем старые значения и добавляем новые для тензодатчика 1
  total1 = total1 - readings1[readIndex];
  readings1[readIndex] = scale1.get_units() * 0.001;  // Перевод в кг
  total1 = total1 + readings1[readIndex];
  average1 = total1 / numReadings;
    if (average1 < 0){
    average1 = 0.0;
  }

  // Для тензодатчика 2
  total2 = total2 - readings2[readIndex];
  readings2[readIndex] = scale2.get_units() * 0.001;  // Перевод в кг
  total2 = total2 + readings2[readIndex];
  average2 = total2 / numReadings;
  if (average2 < 0){
    average2 = 0.0;
  }

  // Для тензодатчика 3
  total3 = total3 - readings3[readIndex];
  readings3[readIndex] = scale3.get_units() * 0.001;  // Перевод в кг
  total3 = total3 + readings3[readIndex];
  average3 = total3 / numReadings;
  if (average3 < 0){
    average3 = 0.0;
  }

  // Для тензодатчика 4
  total4 = total4 - readings4[readIndex];
  readings4[readIndex] = scale4.get_units() * 0.001;  // Перевод в кг
  total4 = total4 + readings4[readIndex];
  average4 = total4 / numReadings;
  if (average4 < 0){
    average4 = 0.0;
  }

  // Для тензодатчика 5
  total5 = total5 - readings5[readIndex];
  readings5[readIndex] = scale5.get_units() * 0.001;  // Перевод в кг
  total5 = total5 + readings5[readIndex];
  average5 = total5 / numReadings;
  if (average5 < 0){
    average5 = 0.0;
  }

  // Обновляем индекс для следующего измерения
  readIndex = (readIndex + 1) % numReadings;

  // Вывод усредненных значений
  Serial.print("Тензодатчик 1 (кг): ");
  Serial.print(scale1.get_units() * 0.001, 4);
  Serial.print(" | ");

  Serial.print("Тензодатчик 2 (кг): ");
  Serial.print(scale2.get_units() * 0.001, 4);
  Serial.print(" | ");

  Serial.print("Тензодатчик 3 (кг): ");
  Serial.print(scale3.get_units() * 0.001, 4);
  Serial.print(" | ");

  Serial.print("Тензодатчик 4 (кг): ");
  Serial.print(scale4.get_units() * 0.001, 4);
  Serial.print(" | ");

  Serial.print("Тензодатчик 5 (кг): ");
  Serial.print(scale5.get_units() * 0.001, 4);
  Serial.println(" ");

  delay(100);
}