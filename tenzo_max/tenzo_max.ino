#include <HX711.h>

HX711 scale1;  // Тензодатчик для измерения момента
HX711 scale2;  // Тензодатчик 2 (при необходимости)
HX711 scale3;  // Тензодатчик 3 (при необходимости)

// Переменные для хранения максимальных значений
float maxMoment = 0;
float currentMoment = 0;

// Массив для хранения максимальных моментов
const int arraySize = 10;  // Размер массива
float moments[arraySize];    // Массив для хранения максимальных значений
int index = 0;              // Индекс для записи в массив
int count = 0;              // Счетчик записанных значений

// Переменная для усредненного значения
float average = 0;

// Порог для различия значений
const float threshold = 0.1; // Например, 0.1 для 10% различия

void setup() {
    Serial.begin(115200);

    scale1.begin(4, 3);
    scale2.begin(6, 5);
    scale3.begin(7, 8);

    scale1.set_scale(530);
    scale2.set_scale(530);
    scale3.set_scale(224);

    scale1.tare();
    scale2.tare();
    scale3.tare();

    // Инициализация массива нулями
    for (int i = 0; i < arraySize; i++) {
        moments[i] = 0;
    }
}

void loop() {
    // Получаем текущее значение с тензодатчиков
    currentMoment = scale1.get_units() * 0.001;

    // Проверяем, является ли текущее значение максимальным
    if (currentMoment > maxMoment) {
        maxMoment = currentMoment;  // Обновляем максимальное значение
    }

    // Проверяем, произошло ли резкое снижение после максимального значения
    if (maxMoment > 0.2 && maxMoment < 2 && currentMoment < (maxMoment * 0.1)) {
        // Если произошло резкое снижение, добавляем максимальное значение в массив
        // Проверяем, отличается ли новое значение от последнего
        if (count == 0 || fabs(moments[index] - maxMoment) > threshold) {
            moments[index] = maxMoment; // Добавляем максимальное значение в массив

            // Увеличиваем счетчик, если значение добавлено
            if (count < arraySize) {
                count++; // Увеличиваем счетчик только если не достигнут лимит
            }

            // Обнуляем максимальное значение
            maxMoment = 0;

            // Переход к следующему индексу
            index = (index + 1) % arraySize;  // Циклический индекс для массива

            // Рассчитываем усредненное значение массива
            float sum = 0;
            for (int i = 0; i < arraySize; i++) {
                sum += moments[i];
            }
            average = sum / count;  // Усредненное значение учитывает только записанные значения

            // Выводим информацию
            Serial.print("Максимальный момент зафиксирован: ");
            Serial.print(moments[(index - 1 + arraySize) % arraySize] * 1.1536,4);  // Выводим последнее зафиксированное значение
            Serial.print("     Усредненное значение: ");
            Serial.println(average,4);
        }
    }

    // Небольшая задержка, чтобы не загромождать вывод
    delay(100);
}