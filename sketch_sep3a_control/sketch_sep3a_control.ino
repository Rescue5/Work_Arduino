#include <Servo.h>
#include <avr/sleep.h>

Servo esc;

int escPin = A2;
int minThrottle = 1000;
int maxThrottle = 2000;
int currentSpeed = minThrottle;

bool testComplete = false;

struct LogData {
  unsigned long timestamp;
  int speed;
};

LogData logData;

void setup() {
  Serial.begin(9600);
  esc.attach(escPin);
  esc.writeMicroseconds(minThrottle);
  
  Serial.println("System Ready. Use 1-9 to control speed, - to stop and enter sleep mode.");
}

void loop() {
  while (Serial.available() > 0) {
    char command = Serial.read();
    processCommand(command);
  }

  if (testComplete) {
    enterSleepMode();
  }
}

void processCommand(char command) {
  if (command >= '1' && command <= '9') {
    int targetSpeed = minThrottle + (command - '1' + 1) * (maxThrottle - minThrottle) / 10;
    adjustSpeed(targetSpeed);
  } else if (command == '-') {
    adjustSpeed(minThrottle);
    Serial.println("Test complete. Entering sleep mode...");
    testComplete = true;
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
  logData.speed = speed;
  logData.timestamp = millis();
  logDataToSerial(logData);
}

void logDataToSerial(const LogData& data) {
  Serial.print("timestamp,");
  Serial.print(data.timestamp);
  Serial.print(",speed,");
  Serial.println(data.speed);
}

void enterSleepMode() {
  Serial.flush();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  // Выйти из режима сна можно только с помощью сброса
}
