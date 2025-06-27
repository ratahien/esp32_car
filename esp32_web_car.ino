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
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>ESP32 Car Control</title>
    <style>
      :root {
        --bg-color: #f4f7f6;
        --container-bg: white;
        --text-color: #333;
        --primary-btn-bg: #007bff;
        --stop-btn-bg: #dc3545;
        --water-btn-bg: #28a745;
        --servo-btn-bg: #ffc107;
        --shadow-light: rgba(0, 0, 0, 0.1);
        --shadow-medium: rgba(0, 0, 0, 0.2);
      }

      /* Sử dụng box-sizing toàn cục để kiểm soát kích thước dễ hơn */
      html {
        box-sizing: border-box;
      }
      *,
      *:before,
      *:after {
        box-sizing: inherit;
      }

      body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto,
          Helvetica, Arial, sans-serif;
        text-align: center;
        margin: 0;
        background-color: var(--bg-color);
        /* Bỏ padding ở body để container có thể chiếm toàn bộ không gian */
      }

      .container {
        width: 100vw; /* Chiếm toàn bộ chiều rộng */
        height: 100svh; /* Chiếm toàn bộ chiều cao thực tế của viewport */
        margin: 0;
        background: var(--container-bg);
        padding: clamp(10px, 2svh, 20px); /* Padding theo chiều cao màn hình */
        border-radius: 0; /* Bỏ border-radius khi full-screen */
        box-shadow: none;

        /* Cấu trúc flex để quản lý không gian dọc */
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: flex-start; /* Bắt đầu từ trên xuống */
        overflow: hidden; /* Cốt lõi: Cấm cuộn trang trong container */
      }

      h1 {
        color: var(--text-color);
        margin: 0 0 clamp(5px, 2svh, 15px) 0;
        font-size: clamp(
          1.5em,
          5svh,
          2.2em
        ); /* Kích thước font theo chiều cao */
        flex-shrink: 0; /* Không cho phép tiêu đề bị co lại */
      }

      .control-sections-wrapper {
        display: flex;
        flex-direction: column;
        width: 100%;
        align-items: center;
        justify-content: center;
        gap: clamp(15px, 2svh, 20px);

        /* Cốt lõi: Cho phép wrapper này lấp đầy không gian còn lại */
        flex-grow: 1;
        min-height: 0; /* Một mẹo flexbox để cho phép co lại đúng cách */
      }

      .car-control-section,
      .pump-servo-section {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center; /* Căn giữa nội dung bên trong */
        padding: clamp(5px, 2vw, 15px);
        border-radius: 8px;
        background-color: #fcfcfc;
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.05);
        width: clamp(280px, 90%, 400px);
      }

      .car-control-grid,
      .servo-control-grid {
        display: grid;
        gap: clamp(8px, 1.5vw, 12px);
        width: 100%;
        max-width: 250px;
      }

      .car-control-grid {
        grid-template-areas:
          ". forward ."
          "left stop right"
          ". backward .";
      }

      .servo-control-grid {
        grid-template-areas:
          ". servo-up ."
          "servo-left water-btn servo-right"
          ". servo-down .";
        max-width: 220px;
      }

      .btn {
        padding: clamp(10px, 2.5vw, 15px);
        font-size: clamp(0.9rem, 2vw, 1rem);
        cursor: pointer;
        border: none;
        border-radius: 8px;
        color: white;
        background-color: var(--primary-btn-bg);
        transition: transform 0.1s ease;
        -webkit-tap-highlight-color: transparent;
      }

      /* Các style khác của .btn giữ nguyên */
      .btn.forward {
        grid-area: forward;
      }
      .btn.left {
        grid-area: left;
      }
      .btn.right {
        grid-area: right;
      }
      .btn.backward {
        grid-area: backward;
      }
      .btn.stop {
        grid-area: stop;
        background-color: var(--stop-btn-bg);
      }
      .btn.water {
        grid-area: water-btn;
        background-color: var(--water-btn-bg);
      }
      .btn.servo {
        background-color: var(--servo-btn-bg);
        color: #333;
      }
      .btn.servo-up {
        grid-area: servo-up;
      }
      .btn.servo-left {
        grid-area: servo-left;
      }
      .btn.servo-right {
        grid-area: servo-right;
      }
      .btn.servo-down {
        grid-area: servo-down;
      }
      .btn:active {
        transform: scale(0.95);
      }

      .status-display {
        font-weight: bold;
        margin-top: 5px;
        color: #555;
        font-size: clamp(0.75rem, 1.8svh, 0.9rem);
      }

      /* --- Media Query CHÍNH cho chế độ màn hình ngang --- */
      @media screen and (orientation: landscape) {
        .container {
          justify-content: center; /* Căn giữa toàn bộ nội dung trong không gian ngang */
        }

        .control-sections-wrapper {
          flex-direction: row; /* Đặt 2 khối điều khiển cạnh nhau */
          align-items: center; /* Căn giữa theo chiều dọc */
          justify-content: space-evenly; /* Phân bố đều không gian */
          gap: 15px;
        }

        .car-control-section,
        .pump-servo-section {
          width: clamp(220px, 45%, 300px); /* Điều chỉnh lại chiều rộng */
          height: 90%; /* Giới hạn chiều cao của các box */
          padding: 10px;
        }

        .car-control-grid,
        .servo-control-grid {
          gap: clamp(5px, 1.5vw, 10px); /* Giảm khoảng cách nút */
          width: 100%;
          height: 100%;
          flex-grow: 1; /* Cho phép grid lấp đầy không gian trong section */
          align-content: center; /* Căn các hàng grid vào giữa */
        }

        .btn {
          padding: clamp(8px, 2vh, 12px); /* Padding theo chiều cao màn hình */
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
            <button
              class="btn forward"
              ontouchstart="sendAction(event, 'forward')"
              ontouchend="sendAction(event, 'stop')"
              onmousedown="sendAction(event, 'forward')"
              onmouseup="sendAction(event, 'stop')"
            >
              Forward
            </button>
            <button
              class="btn left"
              ontouchstart="sendAction(event, 'left')"
              ontouchend="sendAction(event, 'stop')"
              onmousedown="sendAction(event, 'left')"
              onmouseup="sendAction(event, 'stop')"
            >
              Left
            </button>
            <button
              class="btn stop"
              ontouchstart="sendAction(event, 'stop')"
              ontouchend="sendAction(event, 'stop')"
              onmousedown="sendAction(event, 'stop')"
              onmouseup="sendAction(event, 'stop')"
            >
              Stop
            </button>
            <button
              class="btn right"
              ontouchstart="sendAction(event, 'right')"
              ontouchend="sendAction(event, 'stop')"
              onmousedown="sendAction(event, 'right')"
              onmouseup="sendAction('stop')"
            >
              Right
            </button>
            <button
              class="btn backward"
              ontouchstart="sendAction(event, 'backward')"
              ontouchend="sendAction(event, 'stop')"
              onmousedown="sendAction(event, 'backward')"
              onmouseup="sendAction('stop')"
            >
              Backward
            </button>
          </div>
        </div>

        <div class="pump-servo-section">
          <div class="servo-control-grid">
            <button
              class="btn servo servo-up"
              ontouchstart="sendServoAction(event, 'servo1_up')"
              ontouchend="event.preventDefault()"
              onmousedown="sendServoAction(event, 'servo1_up')"
              onmouseup="event.preventDefault()"
            >
              Up
            </button>
            <button
              class="btn servo servo-left"
              ontouchstart="sendServoAction(event, 'servo2_down')"
              ontouchend="event.preventDefault()"
              onmousedown="sendServoAction(event, 'servo2_down')"
              onmouseup="event.preventDefault()"
            >
              Left
            </button>
            <button
              class="btn water"
              ontouchstart="sendPumpAction(event, 'on')"
              ontouchend="sendPumpAction(event, 'off')"
              onmousedown="sendPumpAction(event, 'on')"
              onmouseup="sendPumpAction(event, 'off')"
            >
              Spray Water
            </button>
            <!-- Changed to press-and-hold -->
            <button
              class="btn servo servo-right"
              ontouchstart="sendServoAction(event, 'servo2_up')"
              ontouchend="event.preventDefault()"
              onmousedown="sendServoAction(event, 'servo2_up')"
              onmouseup="event.preventDefault()"
            >
              Right
            </button>
            <button
              class="btn servo servo-down"
              ontouchstart="sendServoAction(event, 'servo1_down')"
              ontouchend="event.preventDefault()"
              onmousedown="sendServoAction(event, 'servo1_down')"
              onmouseup="event.preventDefault()"
            >
              Down
            </button>
          </div>
          <div class="status-display">
            Pump State: <span id="pumpState">Off</span>
          </div>
          <div class="status-display">
            Servo1 (Pan): <span id="servo1State">90</span>&deg;
          </div>
          <div class="status-display">
            Servo2 (Tilt): <span id="servo2State">90</span>&deg;
          </div>
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
        xhr.onreadystatechange = function () {
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
        xhr.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            const response = JSON.parse(this.responseText);
            if (response.servo1_pos !== undefined) {
              document.getElementById("servo1State").innerText =
                response.servo1_pos;
            }
            if (response.servo2_pos !== undefined) {
              document.getElementById("servo2State").innerText =
                response.servo2_pos;
            }
          }
        };
        xhr.open("GET", "/servo_control?action=" + action, true);
        xhr.send();
      }

      // Update all states on page load
      window.onload = function () {
        // Get pump state
        var pumpXhr = new XMLHttpRequest();
        pumpXhr.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            document.getElementById("pumpState").innerText = this.responseText;
          }
        };
        pumpXhr.open("GET", "/getPumpState", true);
        pumpXhr.send();

        // Get servo states
        var servoXhr = new XMLHttpRequest();
        servoXhr.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            const response = JSON.parse(this.responseText);
            document.getElementById("servo1State").innerText =
              response.servo1_pos;
            document.getElementById("servo2State").innerText =
              response.servo2_pos;
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
