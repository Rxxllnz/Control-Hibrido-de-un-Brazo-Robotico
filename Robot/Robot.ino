#include "RobotController.h"
#include <EEPROM.h>
#include <avr/wdt.h>

// Fichero principal del robot
// - Controla 4 servos mediante RobotController
// - Guarda y recupera ángulos en EEPROM usando un botón de interrupción
// - Usa watchdog para recuperar ante bloqueos


// Opción A: por defecto (construido dentro de RobotController)
// RobotController robot; // pines por defecto: servos {9,10,11,12}, pots {A0..A3}, button=2, led=13

// Opción B: proporcionar asignación de pines personalizada desde el .ino
const int customServoPins[RobotController::NUM_SERVOS] = {9, 10, 11, 12};
const int customPotPins[RobotController::NUM_SERVOS]   = {A0, A1, A2, A3};
const int customButtonPin = 2;
const int customLedPin = 22;

const int botonInterrupcion = 3;



// Se crea el controlador con pines personalizados
RobotController robot(customServoPins, customPotPins, customButtonPin, customLedPin);


/*
  manejadorInterrupcion()
  ------------------------
  Rutina de atención a la interrupción (handler) activada por el pin
  `botonInterrupcion`. Se usa para guardar inmediatamente los ángulos
  actuales de los servos en la EEPROM.
*/
void manejadorInterrupcion() {
  Serial.println("Interrupción detectada en el pin del botón");
  Serial.println("Guardando ángulos actuales en EEPROM...");
  const int* angles = robot.getAngles();
  
  for (int i = 0; i < RobotController::NUM_SERVOS; ++i) {
    EEPROM.put(i * sizeof(int), angles[i]);
    Serial.print("Ángulo del servo ");
    Serial.print(i);
    Serial.print(" guardado: ");
    Serial.print(angles[i]);
    Serial.println("°");
  }
}

/*
  recuperarAngulosDeEEPROM()
  ---------------------------
  Lee desde EEPROM los valores guardados anteriormente y los muestra por
  Serial. Mantiene la implementación original: si deseas aplicar los valores
  leídos directamente a los servos al inicio, se puede añadir una función
  pública en `RobotController` para establecerlos.
*/
void recuperarAngulosDeEEPROM() {
  Serial.println("Recuperando ángulos de EEPROM...");
  const int* angles = robot.getAngles();
  for (int i = 0; i < RobotController::NUM_SERVOS; ++i) {
    EEPROM.get(i * sizeof(int), angles[i]);
    Serial.print("Ángulo del servo ");
    Serial.print(i);
    Serial.print(" recuperado: ");
    Serial.print(angles[i]);
    Serial.println("°");
  }
}

void setup() {

  Serial.begin(115200);
  while (!Serial) { ; }

  wdt_disable(); // desactiva watchdog timer

  // Registrar puntero de instancia para que el manejador estático de I2C pueda reenviar a la instancia
  RobotController::instancePtr = &robot;

  recuperarAngulosDeEEPROM();
  robot.begin();


  pinMode(botonInterrupcion, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(botonInterrupcion), manejadorInterrupcion, FALLING);

  wdt_enable(WDTO_1S); // Habilita watchdog timer con tiempo de espera de 1 segundo
  wdt_reset(); // Reinicia watchdog timer
}

void loop() {
  wdt_reset(); // Reinicia watchdog timer
  robot.loopTick();
}

