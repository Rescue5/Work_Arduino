#include <Wire.h>
#include <Adafruit_MCP4725.h>

// Создаем экземпляр ЦАП
Adafruit_MCP4725 dac;

// Адрес ЦАП (по умолчанию 0x60, может быть 0x61)
#define DAC_ADDRESS 0x60

void setup() {
    Serial.begin(115200);        // Инициализация серийного порта
    dac.begin(DAC_ADDRESS);      // Инициализация ЦАП

    Serial.println("Инициализация ЦАП завершена.");
}

void loop() {
  // Установка напряжения от 0 до 4095 (0-5 В)
    for (int value = 0; value <= 4095; value += 500) {
        dac.setVoltage(value, false); // Установить напряжение без сохранения в EEPROM
        Serial.print("Установлено значение ЦАП: ");
        Serial.println(value);
        delay(1000); // Задержка 1 секунда
    }

  // // Установка напряжения обратно от 4095 до 0
  //   for (int value = 4095; value >= 0; value -= 500) {
  //       dac.setVoltage(value, false);
  //       Serial.print("Установлено значение ЦАП: ");
  //       Serial.println(value);
  //       delay(1000); // Задержка 1 секунда
  //   }

  dac.setVoltage(0, false);

  delay(1000); // Дополнительная задержка перед следующим циклом
}