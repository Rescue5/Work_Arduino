#include <Servo.h>

Servo esc;
const int ESC_PIN = A3;

const int STOP_PWM = 1000;
const int MIN_PWM = 1300;
const int MAX_PWM = 1700;

const int SLOW_INC_DELAY = 10;
const int FAST_INC_DELAY = 5;
const int SLOW_DEC_DELAY = 30;

const unsigned long RUN_TIME = 5 * 60 * 1000;  // 5 минут
unsigned long startTime = 0;

int currentPWM = STOP_PWM;
bool isRunning = false;

void setup() {
    Serial.begin(115200);
    esc.attach(ESC_PIN);
    esc.writeMicroseconds(STOP_PWM);  // Остановка
    delay(10000);  // Ждём инициализацию ESC
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        if (command == "START") {
            isRunning = true;
            startTime = millis();  // Запоминаем время старта
            currentPWM = STOP_PWM;  // Сброс текущего PWM

            // 1️⃣ Плавный разгон
            for (int pwm = STOP_PWM; pwm <= MIN_PWM; pwm += 5) {
                esc.writeMicroseconds(pwm);
                currentPWM = pwm;
                delay(SLOW_INC_DELAY);
                Serial.print("Set PWM to ");
                Serial.println(pwm);
            }
        } else if (command == "STOP") {
            stopMotor();
        }
    }

    // Проверка таймаута перед выполнением цикла
    if (isRunning && (millis() - startTime > RUN_TIME)) {
        Serial.println("5 minute timeout reached");
        stopMotor();
        return;
    }

    if (isRunning) {
        // 2️⃣ Быстрый разгон до 70%
        for (int pwm = currentPWM; pwm <= MAX_PWM; pwm += 10) {
            if (!isRunning || (millis() - startTime > RUN_TIME)) {
                stopMotor();
                return;
            }
            esc.writeMicroseconds(pwm);
            currentPWM = pwm;
            delay(FAST_INC_DELAY);
            Serial.print("Set PWM to ");
            Serial.println(pwm);
        }

        if (!isRunning) return;
        delay(3000);  // Удерживаем 3 секунды на 70%
        if (!isRunning) return;

        // 3️⃣ Медленный сброс до 30%
        for (int pwm = currentPWM; pwm >= MIN_PWM; pwm -= 5) {
            if (!isRunning || (millis() - startTime > RUN_TIME)) {
                stopMotor();
                return;
            }
            esc.writeMicroseconds(pwm);
            currentPWM = pwm;
            delay(SLOW_DEC_DELAY);
            Serial.print("Set PWM to ");
            Serial.println(pwm);
        }

        if (!isRunning) return;
        delay(5000);  // Удерживаем 5 секунд на 30%
    }
}

void stopMotor() {
    isRunning = false;
    // 4️⃣ Плавная остановка
    for (int pwm = currentPWM; pwm >= STOP_PWM; pwm -= 10) {
        esc.writeMicroseconds(pwm);
        currentPWM = pwm;
        delay(SLOW_DEC_DELAY);
        Serial.print("Set PWM to ");
        Serial.println(pwm);
    }
    Serial.println("Motor stopped");
}