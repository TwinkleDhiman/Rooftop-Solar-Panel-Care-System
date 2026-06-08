#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>

// WiFi credentials
const char* ssid = "Airtel_94A";
const char* password = "Ramesh2!(^#";

// Web server on port 80
WebServer server(80);

// Motor pins
#define IN1 13
#define IN2 14
#define IN3 27
#define IN4 26
int enA=22;
int enB=23;

// Ultrasonic pins
#define TRIG 5
#define ECHO 18

// IR sensor pin
#define IR_SENSOR 4

// Relay pin
#define RELAY 32

// Control flags
bool isAutoCleaning = false;

// Motor control functions
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// Sensor reading functions
long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH) * 0.034 / 2;
}

bool detectObstacle() {
  long distance = getDistance();
  return (distance > 0 && distance < 20) || digitalRead(IR_SENSOR) == LOW;
}

// Auto cleaning logic
void autoClean() {
  digitalWrite(RELAY, HIGH); // Turn ON relay
  moveForward();
  delay(500);
  if (detectObstacle()) {
    stopMotors();
    delay(300);
    moveBackward();
    delay(400);
    stopMotors();
    delay(300);
    turnLeft();
    delay(500);
    stopMotors();
  }
}

// HTML UI
String htmlPage() {
  return R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Solar Panel Cleaner</title>
      <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            text-align: center;
            background-color: #e0f7fa; /* Light Teal Background */
            margin: 0;
            padding: 40px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }

        h2 {
            color: #00897b; /* Teal Accent Color */
            margin-bottom: 20px;
            font-size: 2.5em;
            text-shadow: 1px 1px #b2dfdb;
        }

        p {
            color: #424242; /* Dark Gray Text */
            font-size: 1.1em;
            margin-bottom: 15px;
        }

        .button-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 20px;
            margin-top: 30px;
            width: 80%;
            max-width: 600px;
        }

        button {
            padding: 15px 25px;
            font-size: 1.2em;
            border: none;
            border-radius: 10px;
            background-color: #00bcd4; /* Cyan Button */
            color: white;
            cursor: pointer;
            transition: transform 0.1s ease-in-out, box-shadow 0.1s ease-in-out;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
        }

        button:hover {
            background-color: #0097a7; /* Darker Cyan on Hover */
            transform: scale(1.05);
            box-shadow: 0 6px 12px rgba(0, 0, 0, 0.15);
        }

        button:active {
            transform: scale(0.98);
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        }

        hr {
            margin: 50px 0;
            border: none;
            border-top: 3px solid #80cbc4; /* Light Teal Divider */
            width: 70%;
        }
    </style>
    </head>
    <body>
      <h2>Solar Panel Cleaner</h2>
      <p>Manual Control</p>
      <button onclick="sendCommand('forward')">Forward</button><br>
      <button onclick="sendCommand('left')">Left</button>
      <button onclick="sendCommand('stop')">Stop</button>
      <button onclick="sendCommand('right')">Right</button><br>
      <button onclick="sendCommand('backward')">Backward</button>
      <hr>
      <p>Mode Control</p>
      <button onclick="sendCommand('auto')">Auto Clean</button>
      <button onclick="sendCommand('manual')">Manual Mode</button>
      <script>
        function sendCommand(cmd) {
          fetch('/' + cmd);
        }
      </script>
    </body>
    </html>
  )rawliteral";
}

// Handle web commands
void handleCommand(String cmd) {
  if (cmd == "forward") {
    isAutoCleaning = false;
    moveForward();
  } else if (cmd == "backward") {
    isAutoCleaning = false;
    moveBackward();
  } else if (cmd == "left") {
    isAutoCleaning = false;
    turnLeft();
  } else if (cmd == "right") {
    isAutoCleaning = false;
    turnRight();
  } else if (cmd == "stop") {
    isAutoCleaning = false;
    stopMotors();
  } else if (cmd == "auto") {
    isAutoCleaning = true;
  } else if (cmd == "manual") {
    isAutoCleaning = false;
    stopMotors();
    digitalWrite(RELAY, LOW); // Turn off relay
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopMotors();

  // Sensor pins
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(IR_SENSOR, INPUT);

  // Relay
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW); // Initially off

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected. IP: " + WiFi.localIP().toString());

  // OTA
  ArduinoOTA.begin();

  // Web routes
  server.on("/", []() { server.send(200, "text/html", htmlPage()); });
  server.on("/forward", []() { handleCommand("forward"); });
  server.on("/backward", []() { handleCommand("backward"); });
  server.on("/left", []() { handleCommand("left"); });
  server.on("/right", []() { handleCommand("right"); });
  server.on("/stop", []() { handleCommand("stop"); });
  server.on("/auto", []() { handleCommand("auto"); });
  server.on("/manual", []() { handleCommand("manual"); });
  server.begin();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  if (isAutoCleaning) {
    autoClean();
  }
}
