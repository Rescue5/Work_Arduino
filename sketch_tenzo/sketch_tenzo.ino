#include <HX711.h>

// Определяем пины
#define DOUT  2  // Подключаем к пину D2 на Arduino
#define CLK   3  // Подключаем к пину D3 на Arduino

HX711 scale;

// Калибровка
void setup() {
  Serial.begin(9600);
  scale.begin(DOUT, CLK);

  Serial.println("Калибровка...");
  scale.set_scale(387.0); // Установите масштаб вручную в зависимости от вашей калибровки
  scale.tare(); // Устанавливаем текущее значение как нулевое
  Serial.println("Тензодатчик калиброван.");
}

void loop() {
  if (scale.is_ready()) {
    long reading = scale.get_units(10); // Получаем усредненное значение (10 измерений)
    Serial.print("Вес: ");
    Serial.println(reading);
  } else {
    Serial.println("HX711 не найден.");
  }
  delay(100);
}