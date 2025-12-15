# Control Híbrido de un Brazo Robótico

## 📋 Descripción del Proyecto

Sistema de control avanzado para un brazo robótico de 4 grados de libertad. El proyecto implementa un control híbrido que permite manejar el brazo mediante:

- **Control local**: Potenciómetros analógicos conectados directamente
- **Control remoto**: Interfaz web a través de WiFi (ESP32)
- **Almacenamiento**: Persistencia de posiciones en EEPROM mediante botón de interrupción

##  Arquitectura del Sistema

El proyecto está dividido en dos componentes principales:

### 1. **Arduino + 4 Servomotores** (`/Robot`)
- **RobotController**: Clase encargada de controlar los 4 servos
- Captura de entrada desde potenciómetros analógicos
- Comunicación I2C con el ESP32
- Almacenamiento de posiciones en EEPROM
- Sistema de filtrado de ruido en lecturas analógicas
- Protección mediante watchdog timer

### 2. **ESP32 - Interfaz Web** (`/ESP32`)
- Servidor web para control remoto
- Conexión WiFi integrada
- Comunicación I2C con Arduino esclavo
- Control de sliders y botones desde navegador
- Modo deep sleep para ahorro de energía

##  Requisitos de Hardware

### Arduino (Control Local)
- Microcontrolador Arduino (ATmega328P o compatible)
- 4 Servomotores MG90S o similares
- 4 Potenciómetros 100kΩ
- 1 Botón de interrupción
- 1 Botón para cambio entre modos Local/Remoto
- 1 LED indicador
- Conexión I2C (SDA/SCL)

### ESP32 (Control Remoto)
- Microcontrolador ESP32
- Conexión WiFi integrada
- Pines I2C configurables (SDA=21, SCL=22)
- Pines capacitivos para control (T3=15, T4=13)

##  Estructura del Proyecto

```
Control-Hibrido-de-un-Brazo-Robotico/
├── Robot/
│   ├── Robot.ino              # Sketch principal del Arduino
│   ├── RobotController.h      # Declaración de la clase
│   └── RobotController.cpp    # Implementación del controlador
├── ESP32/
│   └── ESP32.ino              # Sketch del ESP32 (servidor web)
└── README.md
```

##  Configuración de Pines

### Arduino
| Componente         | Pines          |
|--------------------|----------------|
| Servos             | 9, 10, 11, 12  |
| Potenciómetros     | A0, A1, A2, A3 |
| Botón Interrupción | 2              |
| Botón Deep Sleep   | 3              |
| LED Indicador      | 22             |

### ESP32
| Componente         | Pines          |
|--------------------|----------------|
| I2C SDA            | 21             |
| I2C SCL            | 22             |
| Touch Control 1    | 15 (T3)        |
| Touch Control 2    | 13 (T4)        |

##  Instalación y Uso

### Arduino
1. Abre `Robot/Robot.ino` en Arduino IDE
2. Verifica la configuración de pines en la sección de constantes
3. Sube el código al Arduino
4. Conecta los servos, potenciómetros y botones según la configuración de pines

### ESP32
1. Abre `ESP32/ESP32.ino` en Arduino IDE
2. **Configura las credenciales WiFi** en las constantes:
   ```cpp
   const char* ssid = "Tu_SSID";
   const char* password = "Tu_Contraseña";
   ```
3. Verifica los pines I2C (SDA=21, SCL=22)
4. Sube el código al ESP32
5. Accede a la interfaz web desde tu navegador en la dirección IP del ESP32

### Conexión I2C
Conecta el Arduino y el ESP32 a través de los pines SDA y SCL:
- Arduino SDA → ESP32 GPIO 21
- Arduino SCL → ESP32 GPIO 22
- GND común para ambos dispositivos

##  Modos de Control

### Modo Local (Arduino + Potenciómetros)
- Gira los potenciómetros para controlar cada servo
- Los ángulos se actualizan en tiempo real
- Presiona el botón de interrupción para guardar la posición actual en EEPROM

### Modo Remoto (ESP32 + WiFi)
- Accede a la interfaz web desde cualquier navegador
- Usa los sliders para controlar cada servo
- Los valores se envían por I2C al Arduino cada 200ms
- Presiona el botón web para guardar la posición

## Características Técnicas

- **Comunicación I2C**: Dirección 0x08 (configurable)
- **Filtrado de Ruido**: Filtro de media móvil en lecturas analógicas
- **Debounce**: 50ms para entrada de botones
- **Intervalo de Envío**: 200ms mínimo entre transmisiones I2C
- **EEPROM**: Almacenamiento persistente de ángulos de servos
- **Watchdog Timer**: Recuperación automática ante bloqueos
- **Deep Sleep**: Modo de bajo consumo en ESP32

##  Notas Importantes

1. **Rango de Ángulos**: Los servos típicamente aceptan rangos de 0-180°
2. **Calibración**: Ajusta los filtros y valores de potenciómetros según tu hardware
3. **Sincronización**: El Arduino es maestro en actualizaciones locales, ESP32 es cliente remoto
4. **Seguridad WiFi**: Cambia las credenciales antes de usar

##  Solución de Problemas

| Problema               | Solución                                      |
|------------------------|-----------------------------------------------|
| Servo no responde      | Verifica pines de alimentación y conexión I2C |
| WiFi no conecta        | Revisa SSID/contraseña en ESP32.ino           |
| Comunicación I2C falla | Confirma pines SDA/SCL y dirección (0x08)     |
| Lecturas ruidosas      | Aumenta el valor de `filterValue`             |
| Arduino se bloquea     | El watchdog lo reiniciará automáticamente     |

##  Referencias

- [Arduino Servo Library](https://www.arduino.cc/en/Reference/Servo)
- [Wire Library (I2C)](https://www.arduino.cc/en/Reference/Wire)
- [ESP32 WiFi](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)

##  Licencia

Proyecto de sistemas embebidos y robótica.

---

**Autor**: Raúl Lorenzo Parrado  
**Última actualización**: Diciembre 2025