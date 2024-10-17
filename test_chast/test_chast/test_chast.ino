#define SPEED_PIN 11

void setup() {
  Serial.begin(115200);
  pinMode(SPEED_PIN, OUTPUT); // шим

  analogWrite(SPEED_PIN, 200);
  delay(1000);
  Serial.println(50);
}

void loop() {
  // analogWrite(SPEED_PIN, 10);
  // delay(1000);
  // Serial.println(10);
  // analogWrite(SPEED_PIN, 20);
  // delay(1000);
  // Serial.println(20);
  // analogWrite(SPEED_PIN, 30);
  // delay(1000);
  // Serial.println(30);
  // analogWrite(SPEED_PIN, 40);
  // delay(1000);
  // Serial.println(40);
  // analogWrite(SPEED_PIN, 50);
  // delay(1000);
  // Serial.println(50);
  // analogWrite(SPEED_PIN, 40);
  // delay(1000);
  // Serial.println(40);
  // analogWrite(SPEED_PIN, 30);
  // delay(1000);
  // Serial.println(30);
  // analogWrite(SPEED_PIN, 20);
  // delay(1000);
  // Serial.println(20);
  // analogWrite(SPEED_PIN, 10);
  // delay(1000);
  // Serial.println(10);
  // analogWrite(SPEED_PIN, 0);
  // delay(1000);
  // Serial.println(0);
}
