// RobotController.h
#ifndef ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_H

#include <Arduino.h>
#include <Servo.h>

// Dirección I2C esperada (coincide con ESP32)
static const uint8_t I2C_ADDRESS = 0x08;

class RobotController {
public:
  static const int NUM_SERVOS = 4;

  RobotController();
  // Constructor alternativo: pasar asignaciones de pines personalizadas (los arrays deben tener NUM_SERVOS elementos)
  RobotController(const int servoPinsIn[NUM_SERVOS], const int potPinsIn[NUM_SERVOS], int buttonPinIn, int ledPinIn);
  void begin();
  void loopTick();

  // Accesores para los ángulos actuales
  const int* getAngles() const;

  // Estado I2C accesible si es necesario
  static volatile bool i2cDataAvailable;
  static volatile uint8_t i2cBuf[4];
  static RobotController* instancePtr;

private:
  enum Mode { MODE_POTS = 0, MODE_SERIAL = 1 };

  int servoPins[NUM_SERVOS];
  int potPins[NUM_SERVOS];
  int buttonPin;
  int ledPin;

  Servo servos[NUM_SERVOS];
  int potValues[NUM_SERVOS];
  int angles[NUM_SERVOS];
  int discardCount[NUM_SERVOS];

  // debounce
  unsigned long lastDebounceTime;
  const unsigned long debounceDelay = 50;
  int lastButtonReading;
  bool buttonPressedHandled;

  //Filtering
  int lastPotValues[NUM_SERVOS] = {0,0,0,0};
  int filterValue = 4;

  const int MAX_POT_DELTA = 20;
  Mode currentMode;

  // helpers
  void initServos();
  void initIO();
  void handleButton();
  void toggleMode();
  void updatePotsMode();
  void updateI2CMode();
  void printAngles();

  // I2C receive handler (no debe registrarse directamente en Wire)
  void onReceive(int howMany);
  static void onReceiveStatic(int howMany);

  // permitir que setup() use internals si hace falta
  friend void ::setup();
};

#endif // ROBOT_CONTROLLER_H
