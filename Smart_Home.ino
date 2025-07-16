#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "id wifi";
const char* password = "password";

WebServer server(80);
Servo servo1, servo2;

// Pin Definitions
#define RAIN_PIN 34
#define TRIG_PIN 5
#define ECHO_PIN 18
#define TEMP_PIN 35
#define LDR_PIN 32
#define RELAY_PIN 27
#define LED1 14  // Kamar 1
#define LED2 12  // Kamar 2
#define LED3 13  // Ruang Tamu
#define LED4 15  // Teras

bool manualMode = false;
int servo1Pos = 0;
int servo2Pos = 0;
int relayState = 0;
int ledState[4] = {0, 0, 0, 0};

// Fungsi baca sensor
float readTemp() {
  int adc = analogRead(TEMP_PIN);
  float voltage = (adc / 4095.0) * 3.3;
  return voltage * 100.0;
}

float readDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH) * 0.034 / 2;
}

// Fungsi untuk menampilkan data sensor real-time
String sensorData() {
  float suhu = readTemp();
  float jarak = readDistance();
  int hujan = analogRead(RAIN_PIN);
  int ldr = analogRead(LDR_PIN);

  String data = "<ul>";
  data += "<li>Suhu: " + String(suhu, 1) + " °C</li>";
  data += "<li>Jarak: " + String(jarak, 1) + " cm</li>";
  data += "<li>Sensor Hujan: " + String(hujan) + "</li>";
  data += "<li>Sensor LDR: " + String(ldr) + "</li>";
  data += "</ul>";
  return data;
}

// Fungsi halaman HTML utama
String htmlPage() {
  String modeStr = manualMode ? "Manual" : "Otomatis";
  String html = R"rawliteral(
    <!DOCTYPE html><html>
    <head><meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; background: #f4f4f4; padding: 20px; }
      h2 { text-align: center; }
      .section { background: #fff; padding: 15px; margin-bottom: 15px; border-radius: 10px; box-shadow: 2px 2px 10px #ccc; }
      .btn { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; background: #2196F3; color: white; cursor: pointer; }
      .btn:hover { background: #0b7dda; }
      .slider { width: 100%; }
      .row { display: flex; flex-wrap: wrap; justify-content: space-around; }
    </style>
    </head>
    <body>
      <h2>Smart Home ESP32 Control</h2>
      <div class='section'>
        <h3>Mode: <b>)rawliteral";
  html += modeStr;
  html += R"rawliteral(</b></h3>
        <form action="/toggleMode"><button class="btn">Toggle Mode</button></form>
      </div>
      <div class='section'>
        <h3>Servo 1 (Jemuran) & Servo 2 (Garasi)</h3>
        <form action="/servo">
          Servo1: <input type="range" name="s1" min="0" max="180" value=")rawliteral";
  html += servo1Pos;
  html += R"rawliteral(" class="slider"><br>
          Servo2: <input type="range" name="s2" min="0" max="180" value=")rawliteral";
  html += servo2Pos;
  html += R"rawliteral(" class="slider"><br>
          <button class="btn" type="submit">Update</button>
        </form>
      </div>
      <div class='section'>
        <h3>Control Kipas (Temp > 35°C)</h3>
        <p>Status: <b>)rawliteral";
  html += (relayState ? "ON" : "OFF");
  html += R"rawliteral(</b></p>
        <form action="/relay"><button class="btn" name="r" value="toggle">Toggle Relay</button></form>
      </div>
      <div class='section'>
        <h3>Control Lampu Rumah</h3>
        <div class="row">
          <form action="/led"><button class="btn" name="l" value="0">Kamar 1: )rawliteral";
  html += (ledState[0] ? "ON" : "OFF");
  html += R"rawliteral(</button></form>
          <form action="/led"><button class="btn" name="l" value="1">Kamar 2: )rawliteral";
  html += (ledState[1] ? "ON" : "OFF");
  html += R"rawliteral(</button></form>
          <form action="/led"><button class="btn" name="l" value="2">Ruang Tamu: )rawliteral";
  html += (ledState[2] ? "ON" : "OFF");
  html += R"rawliteral(</button></form>
          <form action="/led"><button class="btn" name="l" value="3">Teras: )rawliteral";
  html += (ledState[3] ? "ON" : "OFF");
  html += R"rawliteral(</button></form>
        </div>
      </div>
      <div class='section'>
        <h3>Monitoring Sensor Real-Time</h3>
        <div id="sensorData">Memuat data...</div>
      </div>

      <script>
        setInterval(() => {
          fetch('/data')
            .then(res => res.text())
            .then(html => {
              document.getElementById('sensorData').innerHTML = html;
            });
        }, 2000);
      </script>
    </body></html>
  )rawliteral";
  return html;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connected");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());

  servo1.attach(23);
  servo2.attach(19);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED1, OUTPUT); pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT); pinMode(LED4, OUTPUT);

  server.on("/", []() {
    server.send(200, "text/html", htmlPage());
  });

  server.on("/toggleMode", []() {
    manualMode = !manualMode;
    server.send(200, "text/html", htmlPage());
  });

  server.on("/servo", []() {
    if (manualMode) {
      if (server.hasArg("s1")) servo1Pos = server.arg("s1").toInt();
      if (server.hasArg("s2")) servo2Pos = server.arg("s2").toInt();
    }
    server.send(200, "text/html", htmlPage());
  });

  server.on("/relay", []() {
    relayState = !relayState;
    server.send(200, "text/html", htmlPage());
  });

  server.on("/led", []() {
    int idx = server.arg("l").toInt();
    ledState[idx] = !ledState[idx];
    int ledPins[] = {LED1, LED2, LED3, LED4};
    digitalWrite(ledPins[idx], ledState[idx]);
    server.send(200, "text/html", htmlPage());
  });

  server.on("/data", []() {
    server.send(200, "text/html", sensorData());
  });

  server.begin();
}

void loop() {
  server.handleClient();

  float suhu = readTemp();
  float jarak = readDistance();
  int hujan = analogRead(RAIN_PIN);
  int ldr = analogRead(LDR_PIN);

  Serial.println("======= MONITORING =======");
  Serial.println("Mode: " + String(manualMode ? "Manual" : "Otomatis"));
  Serial.println("Suhu: " + String(suhu, 2) + "°C");
  Serial.println("Jarak: " + String(jarak, 2) + " cm");
  Serial.println("Sensor Hujan: " + String(hujan));
  Serial.println("Sensor LDR: " + String(ldr));
  Serial.println("Relay: " + String(relayState ? "ON" : "OFF"));
  Serial.println("Servo1: " + String(servo1Pos));
  Serial.println("Servo2: " + String(servo2Pos));
  Serial.print("LEDs: ");
  for (int i = 0; i < 4; i++) Serial.print(ledState[i] ? "ON " : "OFF ");
  Serial.println("\n==========================\n");

  if (!manualMode) {
    static int lastServo1 = -1;
    int targetServo1 = (hujan < 2500) ? 90 : 0;
    if (abs(targetServo1 - lastServo1) >= 3) {
      servo1.write(targetServo1);
      lastServo1 = targetServo1;
    }

    servo2.write(jarak < 17.0 ? 90 : 0);
    relayState = (suhu > 35.0) ? 1 : 0;
    digitalWrite(RELAY_PIN, relayState);

    bool lightOn = (ldr < 1500);
    digitalWrite(LED1, lightOn);
    digitalWrite(LED2, lightOn);
    digitalWrite(LED3, lightOn);
    digitalWrite(LED4, lightOn);
  } else {
    digitalWrite(RELAY_PIN, relayState ? LOW : HIGH); // aktif low
    servo1.write(servo1Pos);
    servo2.write(servo2Pos);
    digitalWrite(LED1, ledState[0]);
    digitalWrite(LED2, ledState[1]);
    digitalWrite(LED3, ledState[2]);
    digitalWrite(LED4, ledState[3]);
  }

  delay(1000);
}
