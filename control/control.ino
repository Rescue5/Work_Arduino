#include <Servo.h>
#include <avr/sleep.h>

Servo esc;

int escPin = A2;
int minThrottle = 1000;
int maxThrottle = 2000;
int currentSpeed = minThrottle;

bool testComplete = false;

void setup() {
  Serial.begin(9600);
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);
  
  Serial.println("System Ready. Use 0-9 to control speed and 10 to set 100%, - to stop and enter sleep mode.");
}

void loop() {
  while (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }

  if (testComplete) {
    enterSleepMode();
  }
}

void processCommand(const String& command) {
  if (command == "-") {
    adjustSpeed(minThrottle);
    Serial.println("Test complete. Entering sleep mode...");
    testComplete = true;
  } else if (command == "10") {
    adjustSpeed(maxThrottle);
  } else if (command.length() == 1 && command >= "0" && command <= "9") {
    int index = command.toInt();
    int targetSpeed = minThrottle + index * (maxThrottle - minThrottle) / 10; // 0-10 соответствует 0%-100%
    adjustSpeed(targetSpeed);
  } else {
    Serial.println("Unknown command.");
  }
}

void adjustSpeed(int targetSpeed) {
  int increment = (targetSpeed > currentSpeed) ? 10 : -10;
  while (currentSpeed != targetSpeed) {
    currentSpeed += increment;
    if ((increment > 0 && currentSpeed > targetSpeed) || (increment < 0 && currentSpeed < targetSpeed)) {
      currentSpeed = targetSpeed;
    }
    setSpeed(currentSpeed);
    delay(100);
  }
}

void setSpeed(int speed) {
  esc.writeMicroseconds(speed);
  Serial.print("Speed set to: ");
  Serial.println(speed);
}

void enterSleepMode() {
  Serial.flush();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  // Выйти из режима сна можно только с помощью сброса
}