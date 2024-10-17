#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Параметры дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Используем -1 если у дисплея нет пина сброса
#define SCREEN_ADDRESS 0x3C  // Адрес I2C дисплея (может быть 0x3D)

// Создаем объект дисплея
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Пины для датчика Холла, датчика напряжения и датчика тока
#define HALL_PIN_D0 2
#define VOLTAGE_PIN A1
#define CURRENT_PIN A2

// Количество магнитов на диске
#define MAGNET_COUNT 7

// Параметры делителя напряжения
const float REFERENCE_VOLTAGE = 5.0;  // Опорное напряжение микроконтроллера (5 В)
const float MAX_INPUT_VOLTAGE = 25.0;  // Максимальное измеряемое входное напряжение
const float DIVIDER_RATIO = MAX_INPUT_VOLTAGE / REFERENCE_VOLTAGE;  // Коэффициент деления

volatile int pulseCount = 0;
unsigned long previousMillis = 0;
const long interval = 1000;  // Интервал измерений в миллисекундах
float voltage = 0;
float rpm = 0;
float current = 0;
float maxKV = 0;
float maxKV_AverageCurrent = 0;
char currentStr[10]; // Объявляем глобально

// Массив для хранения значений тока за последние 2 секунды
const int numReadings = 2;
float currentReadings[numReadings] = {0.0};
int currentReadingIndex = 0;
float totalCurrent = 0.0;
float averageCurrent = 0.0;

void setup() {
  Serial.begin(9600);
  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);

  // Инициализация дисплея
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Ошибка инициализации дисплея SSD1306"));
    for (;;);  // Бесконечный цикл в случае ошибки
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    // Переводим импульсы в полные обороты в минуту (RPM)
    float revolutions = pulses / float(MAGNET_COUNT);  // Количество полных оборотов за секунду
    rpm = revolutions * 60*0,9859;  // Переводим в обороты за минуту
    
    // Считываем напряжение и преобразуем его в реальное значение
    int sensorValue = analogRead(VOLTAGE_PIN);
    float measuredVoltage = sensorValue * (REFERENCE_VOLTAGE / 1020.0);  // Напряжение на пине
    voltage = measuredVoltage * DIVIDER_RATIO;  // Преобразуем в реальное входное напряжение
    
    // Считываем ток и преобразуем его в реальное значение
    int currentSensorValue = analogRead(CURRENT_PIN);
    float voltageAtCurrentPin = currentSensorValue * (REFERENCE_VOLTAGE / 1023.0);  // Напряжение на пине датчика тока
    current = (voltageAtCurrentPin - 2.5) / 0.066;  // Преобразуем в ток (для ACS712 5A: 185 мВ на 1А, сдвиг 2.5В)

    // Преобразование значения тока в строку с 4 знаками после запятой
    dtostrf(current, 6, 4, currentStr);

    // Обновляем массив значений тока и вычисляем среднее
    totalCurrent -= currentReadings[currentReadingIndex];
    currentReadings[currentReadingIndex] = current;
    totalCurrent += current;
    currentReadingIndex = (currentReadingIndex + 1) % numReadings;
    averageCurrent = totalCurrent / numReadings;

    Serial.print("RPM: ");
    Serial.println(rpm);
    Serial.print("Напряжение: ");
    Serial.println(voltage);
    Serial.print("Средний ток: ");
    Serial.println(averageCurrent);

    // Обновляем максимальное значение KV и сохраняем среднее значение тока при этом максимуме
    if (voltage != 0) {
      float kv = rpm / voltage;
      if (kv > maxKV) {
        maxKV = kv;
        maxKV_AverageCurrent = averageCurrent;
      }
    }
  }

  // Обновляем отображение на дисплее
  display.clearDisplay();
  display.setCursor(0, 0);

  // Выводим данные на дисплей
  display.setTextSize(1);  // Устанавливаем размер шрифта 1 для первых трех строк
  display.print("Об/мин: ");
  display.println(rpm);
  display.print("В: ");
  display.println(voltage);
  display.print("Средн. А: ");
  display.println(averageCurrent);

  display.setTextSize(2);  // Устанавливаем размер шрифта 2 для строки KVmax и макс. тока при этом
  display.setCursor(0, 30);  // Перемещаем курсор ниже для новой строки
  display.print("KVmax: ");
  display.println(maxKV);
  display.print("А при KVmax: ");
  display.println(maxKV_AverageCurrent);

  display.display();
}

void countPulse() {
  pulseCount++;
}
