#include <avr/sleep.h>

// Пин подключения датчика Холла
#define HALL_PIN_D0 2
// Количество импульсов для расчета
#define PULSE_THRESHOLD 70
// Количество оборотов, соответствующих PULSE_THRESHOLD
#define REVS_PER_PULSE_THRESHOLD 10
// Время для сброса (в миллисекундах)
const unsigned long TIME_THRESHOLD = 800; // 0.8 секунды

volatile int pulseCount = 0; // Переменная для подсчёта импульсов
unsigned long lastTime = 0; // Время последнего считывания
unsigned long startTime = 0; // Время начала накопления импульсов
float rpm = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Запуск программы расчёта оборотов двигателя...");

  pinMode(HALL_PIN_D0, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN_D0), countPulse, RISING);
}

void loop() {
  //Serial.println(rpm, 2);

  unsigned long currentMillis = millis();

  // Проверяем, превышает ли время накопления импульсов 0.8 секунды
  if (pulseCount > 0 && (currentMillis - startTime >= TIME_THRESHOLD)) {
    // Сбрасываем количество импульсов и RPM
    pulseCount = 0;
    rpm = 0;
    //Serial.println("Импульсы сброшены. Двигатель, возможно, стоит.");
  }
}

// Прерывание для подсчёта импульсов с датчика Холла
void countPulse() {
  // Если это первый импульс, запоминаем время начала накопления
  if (pulseCount == 0) {
    startTime = millis();
  }

  pulseCount++;

  // Если достигнут порог импульсов
  if (pulseCount >= PULSE_THRESHOLD) {
    // Расчет времени, когда были получены эти импульсы
    unsigned long currentTime = millis();
    unsigned long timeForPulses = currentTime - startTime;

    // Расчет оборотов в минуту
    if (timeForPulses > 0) {
      // RPM = (количество оборотов * 60) / (время в секундах)
      rpm = (REVS_PER_PULSE_THRESHOLD * 60.0) / (timeForPulses / 1000.0);

      // Вывод RPM в последовательный порт
      Serial.print("Текущие обороты (RPM): ");
      Serial.println(rpm, 2); // Вывод с двумя знаками после запятой
    }

    // Сбрасываем количество импульсов и время
    pulseCount = 0;
    startTime = 0; // Сбрасываем время начала накопления
  }
}