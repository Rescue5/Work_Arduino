int sensorPin = 2;  // Пин для подключения оптического датчика
int sensorValue = 0;  // Переменная для хранения состояния датчика

void setup() {
  Serial.begin(115200);  // Инициализация Serial для вывода данных
  pinMode(sensorPin, INPUT);  // Устанавливаем пин датчика как вход
}

void loop() {
  sensorValue = digitalRead(sensorPin);  // Чтение состояния датчика

  // Вывод состояния в Serial Monitor
  if (sensorValue == HIGH) {
    Serial.println(1);  // Если видит объект, выводим 1
  } else {
    Serial.println(0);  // Если не видит, выводим 0
  }
  
  delay(100);  // Задержка в 500 мс для удобного наблюдения за изменениями
}