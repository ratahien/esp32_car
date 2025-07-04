#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>

//const char* ssid = "ESP32_Car_Control";
//String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
//String ssid = "BD_contest_" + chipId;
const char* password = "12345678";

const int motor_in1 = 13;
const int motor_in2 = 12;
const int motor_in3 = 14;
const int motor_in4 = 27;
const int pump_pin = 22;
const int servo1_pin = 4;
const int servo2_pin = 15;

bool pump_state = false;
Servo servo1;
Servo servo2;
int servo1_pos = 90;
int servo2_pos = 90;
const int servo_min_angle = 0;
const int servo_max_angle = 180;
const int servo_step = 5;

AsyncWebServer server(80);

void setMotorDirection(int in1, int in2, int in3, int in4) {
  digitalWrite(motor_in1, in1);
  digitalWrite(motor_in2, in2);
  digitalWrite(motor_in3, in3);
  digitalWrite(motor_in4, in4);
}

void moveForward() {
  Serial.println("Action: Car Forward");
  setMotorDirection(LOW, HIGH, LOW, HIGH);
}

void moveBackward() {
  Serial.println("Action: Car Backward");
  setMotorDirection(HIGH, LOW, HIGH, LOW);
}

void turnLeft() {
  Serial.println("Action: Car Left");
  setMotorDirection(LOW, HIGH, HIGH, LOW);
}

void turnRight() {
  Serial.println("Action: Car Right");
  setMotorDirection(HIGH, LOW, LOW, HIGH);
}

void stopCar() {
  Serial.println("Action: Car Stop");
  setMotorDirection(LOW, LOW, LOW, LOW);
}

void controlPump(bool state) {
  pump_state = state;
  digitalWrite(pump_pin, pump_state ? HIGH : LOW);
  Serial.print("Pump State: ");
  Serial.println(pump_state ? "On" : "Off");
}

void controlServo1(int delta) {
  servo1_pos += delta;
  servo1_pos = constrain(servo1_pos, servo_min_angle, servo_max_angle);
  servo1.write(servo1_pos);
  Serial.print("Action: Servo1 (Pan) to ");
  Serial.println(servo1_pos);
}

void controlServo2(int delta) {
  servo2_pos += delta;
  servo2_pos = constrain(servo2_pos, servo_min_angle, servo_max_angle);
  servo2.write(servo2_pos);
  Serial.print("Action: Servo2 (Tilt) to ");
  Serial.println(servo2_pos);
}

void setup() {
  Serial.begin(115200);

  pinMode(motor_in1, OUTPUT);
  pinMode(motor_in2, OUTPUT);
  pinMode(motor_in3, OUTPUT);
  pinMode(motor_in4, OUTPUT);
  pinMode(pump_pin, OUTPUT);

  stopCar();
  controlPump(false);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo1.attach(servo1_pin, 500, 2500);
  servo2.attach(servo2_pin, 500, 2500);
  servo1.write(servo1_pos);
  servo2.write(servo2_pos);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway_IP(192, 168, 4, 1);
  IPAddress subnet_Mask(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway_IP, subnet_Mask);
  delay(1000);
  uint64_t chipId64 = ESP.getEfuseMac();
  String chipStr = String(chipId64);
  String last4 = chipStr.substring(chipStr.length() - 4);
  String ssid = "ESP32_Car_" + last4;
  WiFi.softAP(ssid.c_str(), password);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>ESP32 Car Control</title>
  <style>
    :root {
      /* Điều chỉnh kích thước nút và khoảng cách linh hoạt hơn */
      --button-size: clamp(80px, 18vh, 150px);
      --gap-size: clamp(10px, 2vw, 20px);
    }
    html, body {
      height: 100%;
      margin: 0;
      overflow: hidden; /* Quan trọng: Ngăn không cho cuộn trang */
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: #f4f7f6;
      display: flex;
      justify-content: center;
      align-items: center;
      text-align: center;
      
      /* Chặn sao chép, quét chữ, kéo thả */
      -webkit-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }
    .content-wrapper {
      display: flex;
      flex-direction: row; /* Sắp xếp các mục theo hàng ngang */
      justify-content: center;
      align-items: center;
      gap: calc(var(--gap-size) * 3); /* Tăng khoảng cách giữa 2 cột */
      padding: var(--gap-size);
      width: 100%;
      height: 100%;
      box-sizing: border-box;
    }
    .control-section {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: var(--gap-size);
    }
    h2 {
      margin: 0;
      color: #333;
      font-size: clamp(1.2rem, 3vh, 2rem);
    }
    .grid {
      display: grid;
      gap: var(--gap-size);
      justify-content: center;
    }
    .car, .servo {
      grid-template-areas:
        ". f ."
        "l s r"
        ". b .";
      /* Servo sử dụng lại layout của car */
      grid-template-areas:
        ". u ."
        "l w r"
        ". d .";
      grid-template-columns: repeat(3, var(--button-size));
    }
    /* Gán lại grid-area cho đúng từng layout */
    .car {
      grid-template-areas: ". f ." "l s r" ". b .";
    }
    .servo {
      grid-template-areas: ". u ." "l w r" ". d .";
    }
    button {
      font-size: clamp(0.9rem, 2.5vh, 1.2rem);
      border: none;
      border-radius: 12px;
      color: white;
      cursor: pointer;
      user-select: none;
      touch-action: manipulation;
      width: var(--button-size);
      height: var(--button-size);
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
      transition: transform 0.1s ease, box-shadow 0.1s ease;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    button:active {
        transform: translateY(2px);
        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .f { grid-area: f; background: #007bff; }
    .b { grid-area: b; background: #007bff; }
    .car .l { grid-area: l; background: #007bff; }
    .car .r { grid-area: r; background: #007bff; }
    .s { grid-area: s; background: #dc3545; }
    .u { grid-area: u; background: #ffc107; color: #333; }
    .d { grid-area: d; background: #ffc107; color: #333; }
    .w { grid-area: w; background: #28a745; }
    .servo .l { grid-area: l; background: #ffc107; color: #333; }
    .servo .r { grid-area: r; background: #ffc107; color: #333; }

    /* Thông báo xoay màn hình */
    #rotate-device-message {
      display: none;
      position: fixed;
      top: 0; left: 0;
      width: 100vw; height: 100vh;
      background-color: rgba(0, 0, 0, 0.9);
      color: white;
      z-index: 9999;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      font-size: 1.5rem;
      padding: 20px;
      box-sizing: border-box;
    }
    #rotate-device-message::before {
      content: '🔄';
      font-size: 4rem;
      margin-bottom: 20px;
    }

    @media (orientation: portrait) {
      .content-wrapper { display: none; }
      #rotate-device-message { display: flex; }
    }
  </style>
</head>
<body ontouchstart="">

  <div class="content-wrapper">
    <div class="control-section">
      <h2>Car Control</h2>
      <div class="grid car">
        <button class="f" ontouchstart="fetch('/forward')" ontouchend="fetch('/stop')" onmousedown="fetch('/forward')" onmouseup="fetch('/stop')">Forward</button>
        <button class="l" ontouchstart="fetch('/left')" ontouchend="fetch('/stop')" onmousedown="fetch('/left')" onmouseup="fetch('/stop')">Left</button>
        <button class="s" ontouchstart="fetch('/stop')" onmousedown="fetch('/stop')">Stop</button>
        <button class="r" ontouchstart="fetch('/right')" ontouchend="fetch('/stop')" onmousedown="fetch('/right')" onmouseup="fetch('/stop')">Right</button>
        <button class="b" ontouchstart="fetch('/backward')" ontouchend="fetch('/stop')" onmousedown="fetch('/backward')" onmouseup="fetch('/stop')">Backward</button>
      </div>
    </div>
    
    <div class="control-section">
      <h2>Servo & Pump</h2>
      <div class="grid servo">
        <button class="u" ontouchstart="startRepeat('/servo2up')" ontouchend="stopRepeat()" onmousedown="startRepeat('/servo2up')" onmouseup="stopRepeat()">Up</button>
        <button class="l" ontouchstart="startRepeat('/servo1left')" ontouchend="stopRepeat()" onmousedown="startRepeat('/servo1left')" onmouseup="stopRepeat()">Left</button>
        <button class="w" ontouchstart="fetch('/pump/on')" ontouchend="fetch('/pump/off')" onmousedown="fetch('/pump/on')" onmouseup="fetch('/pump/off')">Spray</button>
        <button class="r" ontouchstart="startRepeat('/servo1right')" ontouchend="stopRepeat()" onmousedown="startRepeat('/servo1right')" onmouseup="stopRepeat()">Right</button>
        <button class="d" ontouchstart="startRepeat('/servo2down')" ontouchend="stopRepeat()" onmousedown="startRepeat('/servo2down')" onmouseup="stopRepeat()">Down</button>
      </div>
    </div>
  </div>

  <div id="rotate-device-message">
    Vui lòng xoay ngang màn hình để sử dụng
  </div>

  <script>
    let intervalId = null;

    function startRepeat(path) {
      if (intervalId) clearInterval(intervalId);
      fetch(path).catch(err => console.error(err));
      intervalId = setInterval(() => {
        fetch(path).catch(err => console.error(err));
      }, 200);
    }

    function stopRepeat() {
      if (intervalId) {
        clearInterval(intervalId);
        intervalId = null;
      }
    }

    document.addEventListener('contextmenu', event => event.preventDefault());
    document.addEventListener('dragstart', event => event.preventDefault());
  </script>
</body>
</html>
    )rawliteral");
  });

  server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request){ moveForward(); request->send(200, "text/plain", "Forward"); });
  server.on("/backward", HTTP_GET, [](AsyncWebServerRequest *request){ moveBackward(); request->send(200, "text/plain", "Backward"); });
  server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request){ turnLeft(); request->send(200, "text/plain", "Left"); });
  server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request){ turnRight(); request->send(200, "text/plain", "Right"); });
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){ stopCar(); request->send(200, "text/plain", "Stop"); });

  server.on("/servo1left", HTTP_GET, [](AsyncWebServerRequest *request){ controlServo1(-servo_step); request->send(200, "text/plain", "Servo1 -"); });
  server.on("/servo1right", HTTP_GET, [](AsyncWebServerRequest *request){ controlServo1(servo_step); request->send(200, "text/plain", "Servo1 +"); });
  server.on("/servo2up", HTTP_GET, [](AsyncWebServerRequest *request){ controlServo2(servo_step); request->send(200, "text/plain", "Servo2 +"); });
  server.on("/servo2down", HTTP_GET, [](AsyncWebServerRequest *request){ controlServo2(-servo_step); request->send(200, "text/plain", "Servo2 -"); });

  server.on("/pump/on", HTTP_GET, [](AsyncWebServerRequest *request){ controlPump(true); request->send(200, "text/plain", "Pump On"); });
  server.on("/pump/off", HTTP_GET, [](AsyncWebServerRequest *request){ controlPump(false); request->send(200, "text/plain", "Pump Off"); });

  server.begin();
}

void loop() {
  // Async server – no loop code needed
}
