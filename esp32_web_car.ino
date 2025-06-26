#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h> // Include the ESP32 Servo library for ESP32 compatibility

// WiFi Access Point (AP) credentials
const char* ssid = "ESP32_Car_Control";
const char* password = "12345678";

// GPIO pins for motor control (Example with L298N)
const int motor_in1 = 13;
const int motor_in2 = 12;
const int motor_in3 = 14;
const int motor_in4 = 27;
const int motor_ena = 5;   // PWM pin for speed control A
const int motor_enb = 23; // PWM pin for speed control B

// GPIO pin for water pump control
// Changed pump_pin from 2 to 22 to avoid conflicts and potential boot issues with GPIO2
const int pump_pin = 22; // Pin connected to the Relay module

// GPIO pins for servo control
const int servo1_pin = 4; // Servo for horizontal movement (e.g., pan)
const int servo2_pin = 15; // Servo for vertical movement (e.g., tilt)

// Pump state variable
bool pump_state = false; // true = on, false = off

// Servo objects
Servo servo1;
Servo servo2;

// Servo current positions and limits
int servo1_pos = 90; // Initial position for servo 1 (center)
int servo2_pos = 90; // Initial position for servo 2 (center)
const int servo_min_angle = 0;
const int servo_max_angle = 180;
const int servo_step = 5; // Degrees per button press

// Initialize AsyncWebServer on port 80 (HTTP)
AsyncWebServer server(80);

// Motor control function
void setMotorDirection(int in1, int in2, int in3, int in4, int enaPin, int enbPin, int speedA, int speedB) {
  analogWrite(enaPin, speedA);
  analogWrite(enbPin, speedB);
  digitalWrite(motor_in1, in1);
  digitalWrite(motor_in2, in2);
  digitalWrite(motor_in3, in3);
  digitalWrite(motor_in4, in4);
}

void moveForward() {
  Serial.println("Forward");
  setMotorDirection(LOW, HIGH, LOW, HIGH, motor_ena, motor_enb, 200, 200);
}

void moveBackward() {
  Serial.println("Backward");
  setMotorDirection(HIGH, LOW, HIGH, LOW, motor_ena, motor_enb, 200, 200);
}

void turnLeft() {
  Serial.println("Left");
  setMotorDirection(LOW, HIGH, HIGH, LOW, motor_ena, motor_enb, 200, 200);
}

void turnRight() {
  Serial.println("Right");
  setMotorDirection(HIGH, LOW, LOW, HIGH, motor_ena, motor_enb, 200, 200);
}

void stopCar() {
  Serial.println("Stop");
  setMotorDirection(LOW, LOW, LOW, LOW, motor_ena, motor_enb, 0, 0);
}

// Water pump control function
void controlPump(bool state) {
  pump_state = state;
  digitalWrite(pump_pin, pump_state ? HIGH : LOW); // HIGH or LOW depending on your relay type
  Serial.print("Pump: ");
  Serial.println(pump_state ? "On" : "Off");
}

// Servo control functions
void controlServo1(int delta) {
  servo1_pos = servo1_pos + delta;
  if (servo1_pos < servo_min_angle) servo1_pos = servo_min_angle;
  if (servo1_pos > servo_max_angle) servo1_pos = servo_max_angle;
  servo1.write(servo1_pos);
  Serial.print("Servo1: ");
  Serial.println(servo1_pos);
}

void controlServo2(int delta) {
  servo2_pos = servo2_pos + delta;
  if (servo2_pos < servo_min_angle) servo2_pos = servo_min_angle;
  if (servo2_pos > servo_max_angle) servo2_pos = servo_max_angle;
  servo2.write(servo2_pos);
  Serial.print("Servo2: ");
  Serial.println(servo2_pos);
}


void setup() {
  Serial.begin(115200);

  // Configure motor GPIO pins
  pinMode(motor_in1, OUTPUT);
  pinMode(motor_in2, OUTPUT);
  pinMode(motor_in3, OUTPUT);
  pinMode(motor_in4, OUTPUT);
  pinMode(motor_ena, OUTPUT);
  pinMode(motor_enb, OUTPUT);
  stopCar(); // Ensure the car is stopped at startup

  // Configure pump GPIO pin
  pinMode(pump_pin, OUTPUT);
  controlPump(false); // Ensure pump is off at startup

  // Allow allocation of all timers to servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Attach servos and set initial positions
  servo1.setPeriodHertz(50); // Standard 50hz servo
  servo2.setPeriodHertz(50); // Standard 50hz servo
  servo1.attach(servo1_pin, 1000, 2000); // Attach servo1 to pin, min/max microseconds for 0 and 180 degrees
  servo2.attach(servo2_pin, 1000, 2000); // Attach servo2 to pin, min/max microseconds for 0 and 180 degrees
  servo1.write(servo1_pos);
  servo2.write(servo2_pos);

  // Start ESP32 in Access Point (AP) mode
  Serial.print("Setting up AP (Access Point)... ");
  // IP Address for the AP (usually 192.168.4.1 is default for ESP32 AP)
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway_IP(192, 168, 4, 1);
  IPAddress subnet_Mask(255, 255, 255, 0);

  WiFi.softAPConfig(local_IP, gateway_IP, subnet_Mask);
  bool result = WiFi.softAP(ssid, password);
  if(result == true) {
    Serial.println("AP created successfully!");
  } else {
    Serial.println("AP creation failed!");
  }
  
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Define Web Server handlers

  // Main page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>ESP32 Car Control</title>
        <style>
          body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin: 0;
            background-color: #f4f7f6;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            -webkit-user-select: none;
            -ms-user-select: none;
            user-select: none;
            overflow-y: auto;
            padding: 10px;
          }
          .container {
            max-width: 95%;
            width: auto;
            margin: 10px auto;
            background: white;
            padding: 20px;
            border-radius: 12px;
            box-shadow: 0 6px 20px rgba(0,0,0,0.1);
            display: flex;
            flex-direction: column; /* Default to column */
            align-items: center;
            justify-content: flex-start;
            flex-wrap: wrap;
            min-height: auto;
          }
          h1 {
            color: #333;
            margin-bottom: 25px;
            font-size: 2em;
            width: 100%;
            text-align: center;
          }
          .control-sections-wrapper {
            display: flex;
            flex-direction: column; /* Default: stack car and pump/servo sections vertically */
            width: 100%;
            align-items: center;
            justify-content: center;
          }
          .car-control-section,
          .pump-servo-section {
            display: flex;
            flex-direction: column;
            align-items: center;
            margin: 15px 0; /* Vertical margin for spacing */
            padding: 10px;
            border-radius: 8px;
            background-color: #fcfcfc;
            box-shadow: 0 2px 5px rgba(0,0,0,0.05);
            width: 90%; /* Take up most width in portrait */
            max-width: 300px; /* Limit max width */
          }

          .car-control-grid {
            display: grid;
            grid-template-areas:
              ". forward ."
              "left stop right"
              ". backward .";
            gap: 12px;
            margin-bottom: 10px;
            max-width: 250px;
            width: 100%;
          }
          /* Modified servo-control-grid for "Spray Water" in center */
          .servo-control-grid {
            display: grid;
            grid-template-areas:
              ". servo-up ."
              "servo-left water-btn servo-right" /* water-btn in the center */
              ". servo-down .";
            gap: 8px;
            margin-top: 15px;
            max-width: 180px; /* Adjusted max-width to accommodate water button */
            width: 100%;
          }

          .btn {
            padding: 15px;
            font-size: 1em;
            cursor: pointer;
            border: none;
            border-radius: 8px;
            color: white;
            background-color: #007bff;
            transition: background-color 0.3s ease, transform 0.1s ease;
            box-shadow: 0 4px 10px rgba(0, 123, 255, 0.2);
            -webkit-tap-highlight-color: transparent;
            min-width: 70px;
          }
          .btn.forward { grid-area: forward; }
          .btn.left { grid-area: left; }
          .btn.right { grid-area: right; }
          .btn.backward { grid-area: backward; }
          .btn.stop {
            grid-area: stop;
            background-color: #dc3545;
            box-shadow: 0 4px 10px rgba(220, 53, 69, 0.2);
          }
          .btn.water {
            background-color: #28a745;
            box-shadow: 0 4px 10px rgba(40, 167, 69, 0.2);
            grid-area: water-btn; /* Assign to grid area */
            width: auto; /* Let grid manage width */
            margin: 0; /* Remove specific margin for grid item */
          }
          .btn.servo {
            background-color: #ffc107;
            color: #333;
            box-shadow: 0 4px 10px rgba(255, 193, 7, 0.2);
          }
          .btn.servo:hover {
            background-color: #e0a800;
          }

          .btn.servo-up { grid-area: servo-up; }
          .btn.servo-left { grid-area: servo-left; }
          .btn.servo-right { grid-area: servo-right; }
          .btn.servo-down { grid-area: servo-down; }

          .btn:hover {
            background-color: #0056b3;
            transform: translateY(-2px);
          }
          .btn.stop:hover { background-color: #c82333; }
          .btn.water:hover { background-color: #218838; }
          
          .btn:active {
            transform: translateY(0);
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
          }

          .status-display {
            font-weight: bold;
            margin-top: 10px;
            color: #555;
            font-size: 0.9em;
            width: 100%;
            text-align: center;
          }

          /* --- Media Queries for Landscape Mode --- */
          @media screen and (orientation: landscape) and (max-height: 500px) {
            .container {
              flex-direction: column; /* Title always on top, so container is column */
              justify-content: flex-start; /* Align content to top */
              align-items: center; /* Center horizontally */
              padding: 10px;
            }
            h1 {
              font-size: 1.8em; /* Slightly larger title in landscape */
              margin-bottom: 15px;
            }
            .control-sections-wrapper {
              flex-direction: row; /* Car and pump/servo sections side-by-side */
              justify-content: space-around; /* Distribute space */
              align-items: flex-start; /* Align sections to top */
              width: 100%;
              flex-wrap: nowrap; /* Prevent wrapping */
            }
            .car-control-section,
            .pump-servo-section {
              margin: 0 10px; /* Horizontal margin between sections */
              flex-shrink: 0; /* Prevent shrinking */
              width: auto; /* Width auto to fit content */
              min-width: 180px; /* Minimum width for car control */
            }
            .car-control-grid {
              gap: 8px;
              margin-bottom: 5px; /* Adjust spacing */
            }
            .btn {
              padding: 10px;
              font-size: 0.8em;
              min-width: 60px;
            }
            .btn.water {
              width: auto; /* Let grid manage width */
            }
            .servo-control-grid {
              gap: 5px;
              margin-top: 10px;
              max-width: 150px; /* Slightly larger for water button */
            }
            .status-display {
              font-size: 0.75em;
              margin-top: 5px;
            }
          }

          /* Further optimization for very wide screens (e.g., tablets in landscape) */
          @media screen and (min-width: 768px) and (orientation: landscape) {
            .container {
                max-width: 900px;
                padding: 30px;
            }
            h1 {
                font-size: 2.2em;
                margin-bottom: 25px;
            }
            .car-control-section,
            .pump-servo-section {
                padding: 20px;
                margin: 0 20px;
                max-width: none; /* Remove max-width for larger screens */
            }
            .car-control-grid {
                max-width: 300px;
                gap: 15px;
            }
            .btn {
                padding: 15px;
                font-size: 1em;
            }
            .btn.water {
                width: auto;
            }
            .servo-control-grid {
                max-width: 180px;
                gap: 10px;
            }
            .status-display {
                font-size: 0.9em;
            }
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h1>ESP32 Car Control</h1>

          <div class="control-sections-wrapper">
            <div class="car-control-section">
              <div class="car-control-grid">
                <button class="btn forward" ontouchstart="sendAction(event, 'forward')" ontouchend="sendAction(event, 'stop')" onmousedown="sendAction(event, 'forward')" onmouseup="sendAction(event, 'stop')">Forward</button>
                <button class="btn left" ontouchstart="sendAction(event, 'left')" ontouchend="sendAction(event, 'stop')" onmousedown="sendAction(event, 'left')" onmouseup="sendAction(event, 'stop')">Left</button>
                <button class="btn stop" ontouchstart="sendAction(event, 'stop')" ontouchend="sendAction(event, 'stop')" onmousedown="sendAction(event, 'stop')" onmouseup="sendAction(event, 'stop')">Stop</button>
                <button class="btn right" ontouchstart="sendAction(event, 'right')" ontouchend="sendAction(event, 'stop')" onmousedown="sendAction(event, 'right')" onmouseup="sendAction('stop')">Right</button>
                <button class="btn backward" ontouchstart="sendAction(event, 'backward')" ontouchend="sendAction(event, 'stop')" onmousedown="sendAction(event, 'backward')" onmouseup="sendAction('stop')">Backward</button>
              </div>
            </div>

            <div class="pump-servo-section">
              <div class="servo-control-grid">
                <button class="btn servo servo-up" ontouchstart="sendServoAction(event, 'servo1_up')" ontouchend="event.preventDefault()" onmousedown="sendServoAction(event, 'servo1_up')" onmouseup="event.preventDefault()">Up</button>
                <button class="btn servo servo-left" ontouchstart="sendServoAction(event, 'servo2_down')" ontouchend="event.preventDefault()" onmousedown="sendServoAction(event, 'servo2_down')" onmouseup="event.preventDefault()">Left</button>
                <button class="btn water" ontouchstart="sendPumpAction(event, 'on')" ontouchend="sendPumpAction(event, 'off')" onmousedown="sendPumpAction(event, 'on')" onmouseup="sendPumpAction(event, 'off')">Spray Water</button> <!-- Changed to press-and-hold -->
                <button class="btn servo servo-right" ontouchstart="sendServoAction(event, 'servo2_up')" ontouchend="event.preventDefault()" onmousedown="sendServoAction(event, 'servo2_up')" onmouseup="event.preventDefault()">Right</button>
                <button class="btn servo servo-down" ontouchstart="sendServoAction(event, 'servo1_down')" ontouchend="event.preventDefault()" onmousedown="sendServoAction(event, 'servo1_down')" onmouseup="event.preventDefault()">Down</button>
              </div>
              <div class="status-display">Pump State: <span id="pumpState">Off</span></div>
              <div class="status-display">Servo1 (Pan): <span id="servo1State">90</span>&deg;</div>
              <div class="status-display">Servo2 (Tilt): <span id="servo2State">90</span>&deg;</div>
            </div>
          </div>
        </div>

        <script>
          function sendAction(event, action) {
            if (event.cancelable) {
                event.preventDefault();
            }
            console.log("Sending car action: " + action);
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/control?action=" + action, true);
            xhr.send();
          }

          // New function for pump press-and-hold control
          function sendPumpAction(event, state) {
            if (event.cancelable) {
                event.preventDefault();
            }
            console.log("Setting pump state: " + state);
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
              if (this.readyState == 4 && this.status == 200) {
                document.getElementById("pumpState").innerText = this.responseText;
              }
            };
            xhr.open("GET", "/pump_control?action=" + state, true); // Changed endpoint to /pump_control
            xhr.send();
          }

          function sendServoAction(event, action) {
            if (event.cancelable) {
                event.preventDefault();
            }
            console.log("Sending servo action: " + action);
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
              if (this.readyState == 4 && this.status == 200) {
                const response = JSON.parse(this.responseText);
                if (response.servo1_pos !== undefined) {
                    document.getElementById("servo1State").innerText = response.servo1_pos;
                }
                if (response.servo2_pos !== undefined) {
                    document.getElementById("servo2State").innerText = response.servo2_pos;
                }
              }
            };
            xhr.open("GET", "/servo_control?action=" + action, true);
            xhr.send();
          }

          // Update all states on page load
          window.onload = function() {
            // Get pump state
            var pumpXhr = new XMLHttpRequest();
            pumpXhr.onreadystatechange = function() {
              if (this.readyState == 4 && this.status == 200) {
                document.getElementById("pumpState").innerText = this.responseText;
              }
            };
            pumpXhr.open("GET", "/getPumpState", true);
            pumpXhr.send();

            // Get servo states
            var servoXhr = new XMLHttpRequest();
            servoXhr.onreadystatechange = function() {
              if (this.readyState == 4 && this.status == 200) {
                const response = JSON.parse(this.responseText);
                document.getElementById("servo1State").innerText = response.servo1_pos;
                document.getElementById("servo2State").innerText = response.servo2_pos;
              }
            };
            servoXhr.open("GET", "/getServoState", true);
            servoXhr.send();
          };
        </script>
      </body>
      </html>
    )rawliteral");
  });

  // Handler for car control commands
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request){
    String action = request->arg("action");
    if (action == "forward") {
      moveForward();
    } else if (action == "backward") {
      moveBackward();
    } else if (action == "left") {
      turnLeft();
    } else if (action == "right") {
      turnRight();
    } else if (action == "stop") {
      stopCar();
    }
    request->send(200, "text/plain", "OK");
  });

  // New handler for explicit pump control (on/off)
  server.on("/pump_control", HTTP_GET, [](AsyncWebServerRequest *request){
    String action = request->arg("action");
    if (action == "on") {
      controlPump(true);
    } else if (action == "off") {
      controlPump(false);
    }
    request->send(200, "text/plain", pump_state ? "On" : "Off");
  });


  // Handler for servo control commands
  server.on("/servo_control", HTTP_GET, [](AsyncWebServerRequest *request){
    String action = request->arg("action");
    if (action == "servo1_up") {
      controlServo1(servo_step);
    } else if (action == "servo1_down") {
      controlServo1(-servo_step);
    } else if (action == "servo2_up") {
      controlServo2(servo_step);
    } else if (action == "servo2_down") {
      controlServo2(-servo_step);
    }
    String jsonResponse = "{\"servo1_pos\":" + String(servo1_pos) + ",\"servo2_pos\":" + String(servo2_pos) + "}";
    request->send(200, "application/json", jsonResponse);
  });

  // Handler to get current water pump state
  server.on("/getPumpState", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", pump_state ? "On" : "Off");
  });

  // Handler to get current servo states
  server.on("/getServoState", HTTP_GET, [](AsyncWebServerRequest *request){
    String jsonResponse = "{\"servo1_pos\":" + String(servo1_pos) + ",\"servo2_pos\":" + String(servo2_pos) + "}";
    request->send(200, "application/json", jsonResponse);
  });

  // Start Web Server
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Nothing needed in loop() as AsyncWebServer handles requests asynchronously
}
