#include <WiFi.h>
#include <Wire.h>

// ---------------- Configuration ----------------
const char* ssid = "Raulordenador";
const char* password = "SistemasEmbebidos";
const uint8_t ARDUINO_I2C_ADDR = 0x08; // dirección I2C del Arduino esclavo

// I2C pins (ajusta si es necesario)
const int I2C_SDA = 21;
const int I2C_SCL = 22;

// Deep Sleep touch pin
const int TOUCH_PIN_INT = 15; // T3
const int TOUCH_PIN_SLEEP = 13; // T4
#define TOUCH_THRESHOLD 5000 // umbral touch

WiFiServer server(80);

// Últimos valores enviados (sliders 0..2, button 0/1)
int lastSent[4] = {-1, -1, -1, -1};
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 200; // ms mínimo entre envíos

// Helper: envía 4 bytes por I2C
void sendI2C(int s0, int s1, int s2, int btn) {
  Wire.beginTransmission(ARDUINO_I2C_ADDR);
  Wire.write((uint8_t)s0);
  Wire.write((uint8_t)s1);
  Wire.write((uint8_t)s2);
  Wire.write((uint8_t)btn);
  Wire.endTransmission();
}

// Helper: parsea query string y devuelve valor int o fallback
int getQueryValue(const String &q, const String &key, int fallback) {
  int k = q.indexOf(key + "=");
  if (k == -1) return fallback;
  int start = k + key.length() + 1;
  int end = q.indexOf('&', start);
  if (end == -1) end = q.length();
  String v = q.substring(start, end);
  v.trim();
  if (v.length() == 0) return fallback;
  return v.toInt();
}

// Deep Sleep
void deepsleep(){

  Serial.println("ESP32 entrando en modo Deep Sleep. Toca el pin " + String(TOUCH_PIN1) + " para despertar.");
  delay(1000);

  // Inicia el Deep Sleep
  esp_deep_sleep_start();
}

// Manejador de interrupción táctil
void touchInterrupt() {
  delay(200);
  Serial.println("¡Se detectó un toque en el pin táctil!");
}

// Construye y devuelve la página HTML (compacta, con 3 sliders y botón)
String buildPage(int s0, int s1, int s2, int btnState) {
  String page = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  page += "<meta charset=\"utf-8\"><title>ESP32 - Control Sliders</title>";
  page += "<style>body{font-family:Arial;margin:0;padding:20px;background:#f5f5f5} .card{max-width:600px;margin:auto;background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.08)} h1{font-size:20px;margin:0 0 10px} .row{margin:12px 0} .label{font-weight:600;margin-bottom:6px} .value{font-size:18px;color:#2c3e50;margin-left:8px} .btn{display:inline-block;padding:10px 14px;border-radius:6px;border:0;background:#3498db;color:#fff;cursor:pointer} .btn.toggle{background:#2ecc71} .btn.close{background:#e74c3c}</style>";
  page += "<script>let timeout=null;function send(values){var q='sl0='+values[0]+'&sl1='+values[1]+'&sl2='+values[2]+'&btn='+values[3];fetch('/update?'+q).catch(e=>console.log(e));}function onSlider(i,el){document.getElementById('v'+i).innerText=el.value;clearTimeout(timeout);timeout=setTimeout(()=>{let s0=document.getElementById('s0').value;let s1=document.getElementById('s1').value;let s2=document.getElementById('s2').value;let b=(document.getElementById('btn').dataset.state==='1')?1:0;send([s0,s1,s2,b]);},200);}function onSliderChange(i,el){let s0=document.getElementById('s0').value;let s1=document.getElementById('s1').value;let s2=document.getElementById('s2').value;let b=(document.getElementById('btn').dataset.state==='1')?1:0;send([s0,s1,s2,b]);}function toggleButton(){let btn=document.getElementById('btn');let st = btn.dataset.state==='1'?'0':'1';btn.dataset.state=st;btn.innerText = st==='1'?'OPEN':'CLOSE';btn.className = 'btn '+(st==='1'?'toggle':'close');let s0=document.getElementById('s0').value;let s1=document.getElementById('s1').value;let s2=document.getElementById('s2').value;send([s0,s1,s2,(st==='1')?1:0]);}</script>";
  page += "</head><body><div class=\"card\"><h1>ESP32 Control - 3 sliders + Open/Close</h1>";
  // slider 0
  page += "<div class=\"row\"><div class=\"label\">Hombro: <span class=\"value\" id=\"v0\">" + String(s1) + "</span>°</div>";
  page += "<input id=\"s1\" type=\"range\" min=\"0\" max=\"180\" value=\"" + String(s1) + "\" oninput=\"onSlider(0,this)\" onchange=\"onSliderChange(1,this)\" style=\"width:100%\"></div>";
  // slider 1
  page += "<div class=\"row\"><div class=\"label\">Codo: <span class=\"value\" id=\"v1\">" + String(s0) + "</span>°</div>";
  page += "<input id=\"s0\" type=\"range\" min=\"0\" max=\"180\" value=\"" + String(s0) + "\" oninput=\"onSlider(1,this)\" onchange=\"onSliderChange(0,this)\" style=\"width:100%\"></div>";
  // slider 2
  page += "<div class=\"row\"><div class=\"label\">Muñeca: <span class=\"value\" id=\"v2\">" + String(s2) + "</span>°</div>";
  page += "<input id=\"s2\" type=\"range\" min=\"0\" max=\"180\" value=\"" + String(s2) + "\" oninput=\"onSlider(2,this)\" onchange=\"onSliderChange(2,this)\" style=\"width:100%\"></div>";
  // button
  String btnText = (btnState==1)?"OPEN":"CLOSE";
  String btnClass = (btnState==1)?"btn toggle":"btn close";
  page += "<div class=\"row\"><button id=\"btn\" class=\"" + btnClass + "\" data-state=\"" + String(btnState) + "\" onclick=\"toggleButton()\">" + btnText + "</button></div>";
  page += "<p style=\"color:#7f8c8d;margin-top:10px;font-size:13px\">Los sliders envían tras 200ms de inactividad o al soltar. El botón envía inmediatamente.</p>";
  page += "</div></body></html>";
  return page;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 Webserver -> I2C sender starting...");

  // Conectar WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
    if (millis() - start > 20000) break; // evita bloqueo largo
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No se pudo conectar a WiFi, continúa en modo local");
  }

  // Iniciar servidor
  server.begin();

  // Iniciar I2C (ESP32 como maestro)
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C iniciado en SDA=%d SCL=%d -> addr 0x%02X\n", I2C_SDA, I2C_SCL, ARDUINO_I2C_ADDR);

  // Valores por defecto
  lastSent[0] = 90;
  lastSent[1] = 90;
  lastSent[2] = 90;
  lastSent[3] = 0;

  // Configuracion de Deep Sleep
  esp_sleep_enable_touchpad_wakeup();
  touchAttachInterrupt(TOUCH_PIN_INT, touchInterrupt, TOUCH_THRESHOLD); // Configura T0 como fuente de wakeup
  touchAttachInterrupt(TOUCH_PIN_SLEEP, deepsleep, TOUCH_THRESHOLD); // Configura T3 como fuente de wakeup
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Nuevo cliente conectado");
    String req = "";
    String currentLine = "";
    unsigned long start = millis();
    while (client.connected() && (millis() - start) < 2000) {
      if (client.available()) {
        char c = client.read();
        req += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // fin del header, procesar request (req contiene todo el header)
            Serial.println("Request:\n" + req);

            // Si es /update?... parsear query y enviar por I2C
            int idx = req.indexOf("GET ");
            if (idx != -1) {
              int sp = req.indexOf(' ', idx + 4);
              if (sp != -1) {
                String path = req.substring(idx + 4, sp);
                Serial.println("Path: " + path);
                if (path.startsWith("/update")) {
                  // Obtener query después de '?'
                  int qidx = path.indexOf('?');
                  String q = "";
                  if (qidx != -1) q = path.substring(qidx + 1);
                  int s0 = getQueryValue(q, "sl0", lastSent[0]);
                  int s1 = getQueryValue(q, "sl1", lastSent[1]);
                  int s2 = getQueryValue(q, "sl2", lastSent[2]);
                  int btn = getQueryValue(q, "btn", lastSent[3]);
                  // Limitar
                  s0 = constrain(s0, 0, 180);
                  s1 = constrain(s1, 0, 180);
                  s2 = constrain(s2, 0, 180);
                  btn = (btn==0)?0:1;

                  // Enviar I2C si cambian o ha pasado intervalo
                  if (s0 != lastSent[0] || s1 != lastSent[1] || s2 != lastSent[2] || btn != lastSent[3] || (millis() - lastSend) > SEND_INTERVAL) {
                    sendI2C(s0, s1, s2, btn);
                    lastSent[0] = s0; lastSent[1] = s1; lastSent[2] = s2; lastSent[3] = btn;
                    lastSend = millis();
                    Serial.printf("Enviado I2C: %d, %d, %d, btn=%d\n", s0, s1, s2, btn);
                  } else {
                    Serial.println("Sin cambios, no se envía I2C");
                  }
                }
              }
            }

            // Responder con la página (incluye últimos valores para sincronizar)
            int s0 = lastSent[0];
            int s1 = lastSent[1];
            int s2 = lastSent[2];
            int btn = lastSent[3];
            String page = buildPage(s0, s1, s2, btn);

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type: text/html");
            client.println("Connection: close");
            client.println();
            client.print(page);

            break; // salir del loop de lectura del cliente
          } else {
            // nueva linea
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    delay(1);
    client.stop();
    Serial.println("Cliente desconectado");
  }

  // No bloquear loop
  delay(10);
}