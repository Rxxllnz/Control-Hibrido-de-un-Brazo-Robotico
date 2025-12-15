#include "RobotController.h"
#include <Wire.h>

// Definiciones estáticas
volatile bool RobotController::i2cDataAvailable = false;
volatile uint8_t RobotController::i2cBuf[4] = {0,0,0,0};
RobotController* RobotController::instancePtr = nullptr;

RobotController::RobotController()
  : servoPins{9, 10, 11, 12}, potPins{A0, A1, A2, A3},
    buttonPin(2), ledPin(13), lastDebounceTime(0), lastButtonReading(HIGH),
    buttonPressedHandled(false), currentMode(MODE_POTS)
{
  for (int i = 0; i < NUM_SERVOS; ++i) {
    angles[i] = 90;
    potValues[i] = 0;
    discardCount[i] = 0;
  }
}

// Constructor alternativo: copiar pines desde el llamador
RobotController::RobotController(const int servoPinsIn[NUM_SERVOS], const int potPinsIn[NUM_SERVOS], int buttonPinIn, int ledPinIn)
  : buttonPin(buttonPinIn), ledPin(ledPinIn), lastDebounceTime(0), lastButtonReading(HIGH),
    buttonPressedHandled(false), currentMode(MODE_POTS)
{
  for (int i = 0; i < NUM_SERVOS; ++i) {
    servoPins[i] = servoPinsIn[i];
    potPins[i] = potPinsIn[i];
    angles[i] = 90;
    potValues[i] = 0;
    discardCount[i] = 0;
  }
}

void RobotController::begin() {

  initServos();
  initIO();

  // Iniciar I2C (esclavo)
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(RobotController::onReceiveStatic);
  Serial.print("I2C slave iniciado en 0x"); Serial.println(I2C_ADDRESS, HEX);

  Serial.println("Control de 4 servos - Iniciado");
  Serial.println("Modo actual: POTS (potenciometros). Pulsa el boton para cambiar a SERIAL.");
}

void RobotController::loopTick() {
  handleButton();
  if (currentMode == MODE_POTS) updatePotsMode();
  else updateI2CMode();
}

void RobotController::initServos() {
  for (int i = 0; i < NUM_SERVOS; ++i) {
    servos[i].attach(servoPins[i]);
    servos[i].write(angles[i]);
  }
}

void RobotController::initIO() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void RobotController::handleButton() {
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
    buttonPressedHandled = false;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && !buttonPressedHandled) {
      toggleMode();
      buttonPressedHandled = true;
    }
  }
  lastButtonReading = reading;
}

void RobotController::toggleMode() {
  currentMode = (currentMode == MODE_POTS) ? MODE_SERIAL : MODE_POTS;
  if (currentMode == MODE_POTS) {
    Serial.println("Modo cambiado: POTS");
    digitalWrite(ledPin, LOW);
    // Al cambiar a modo POTS, asegurar que la garra esté en 100 (posición deseada)
    if (NUM_SERVOS > 3) {
      angles[3] = 100;
      servos[3].write(100);
      Serial.println("Garra ajustada a 100 al entrar en POTS");
    }
  } else {
    Serial.println("Modo cambiado: SERIAL (I2C)");
    digitalWrite(ledPin, HIGH);
  }
}

void RobotController::updatePotsMode() {
  for (int i = 0; i < NUM_SERVOS; ++i) {
    int raw = analogRead(potPins[i]);
    if (raw > lastPotValues[i] + filterValue || raw < lastPotValues[i] - filterValue) {
      int newAngle = map(raw, 1023, 0, 0, 180);
      int delta = abs(newAngle - angles[i]);
      if (delta > MAX_POT_DELTA) {
        discardCount[i]++;
        if (discardCount[i] % 10 == 1) {
          Serial.print("Descartada lectura atipica en pot "); Serial.print(i);
          Serial.print(" : "); Serial.print(newAngle);
          Serial.print(" (prev "); Serial.print(angles[i]); Serial.println(")");
        }
        continue;
      }
      discardCount[i] = 0;
      angles[i] = newAngle;
      servos[i].write(angles[i]);
      lastPotValues[i] = raw;
    }
  }
  printAngles();
  delay(50);
}

void RobotController::updateI2CMode() {
  if (i2cDataAvailable) {
    noInterrupts();
    uint8_t buf[4];
    for (int i = 0; i < 4; ++i) buf[i] = i2cBuf[i];
    i2cDataAvailable = false;
    interrupts();

    // Imprimir por Serial los valores recibidos desde el ESP32
    Serial.print("Received angles: ");
    Serial.print(buf[0]); Serial.print(",");
    Serial.print(buf[1]); Serial.print(",");
    Serial.print(buf[2]); Serial.print(" btn=");
    Serial.println(buf[3]);

    for (int i = 0; i < 3 && i < NUM_SERVOS; ++i) {
      int a = constrain((int)buf[i], 0, 180);
      angles[i] = a;
      servos[i].write(a);
    }
    int btn = buf[3] ? 1 : 0;
    if (NUM_SERVOS > 3) {
      if (btn) {
        servos[3].write(100);
        angles[3] = 100;
      } else {
        servos[3].write(180);
        angles[3] = 180;
      }
    }
  }
  delay(10);
}

void RobotController::printAngles() {
  Serial.print("Angulos: ");
  for (int i = 0; i < NUM_SERVOS; ++i) {
    Serial.print(angles[i]);
    Serial.print("°");
    if (i < NUM_SERVOS-1) Serial.print(" | ");
  }
  Serial.println();
}

void RobotController::onReceive(int howMany) {
  int i = 0;
  while (Wire.available() && i < 4) {
    i2cBuf[i++] = Wire.read();
  }
  i2cDataAvailable = true;
}

void RobotController::onReceiveStatic(int howMany) {
  if (instancePtr) instancePtr->onReceive(howMany);
}

// Devuelve puntero constante a la matriz interna de ángulos
const int* RobotController::getAngles() const {
  return angles;
}
