#include <Arduino.h>
// Display
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
//#include "sprite.h"
// Webserver
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
// Timer
//#include <TickTwo.h>

// Functions
void IRAM_ATTR handleTouch();
void handleTimer15min();
void handleRoot();
void handleData();

// Display config
#define HARDWARE_TYPE MD_MAX72XX::DR1CR0RR0_HW
#define MAX_DEVICES 4
#define CS_PIN 5
MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Set up EEPROM size
#define ADDR_BRIGHTNESS 0
#define ADDR_ANIMATION_TYPE (ADDR_BRIGHTNESS + sizeof(uint8_t))
#define ADDR_TEXT_VALUE (ADDR_ANIMATION_TYPE + sizeof(uint8_t))

// ESP32 webserver config
const char* ssid = "Display Driveline SW";
const char* password = "Geraldo2024";
AsyncWebServer server(80);; // Create a web server instance on port 80
IPAddress apIP(192, 168, 1, 1);  // IP address for the access point
IPAddress subnet(255, 255, 255, 0);  // Subnet mask for the access point

// Touch config
#define TOUCH_THRESHOLD 20
#define TOUCH0_PIN T8
#define TOUCH1_PIN T3

// Sleep config
#define TIME_SET 900000 // 15*60*1000
#define TIME_DEBOUNCE 250
bool flagSleep = false;
// TickTwo timer15Min(handleTimer15min, TIME_SET);
uint32_t lastTime = 0;

// Global variables
uint8_t brightness, animationType, textType = 0;
uint8_t prevLen = 100;
uint16_t pauseTime = 5000;
uint16_t scrollSpeed = 50;
String textInpt = "Vazio";

void IRAM_ATTR handleTouch() {
  uint32_t currTime = millis();
  if (currTime-lastTime > TIME_DEBOUNCE)
  {
    lastTime = currTime;
    flagSleep = true;
  }  
}

void handleTimer15min() {
  flagSleep = true;
}

void handleRoot(AsyncWebServerRequest *request) {
  String message = R"(
    <!DOCTYPE html>
    <html lang="en">
    <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Web Server</title>
    <style>
    body {
    font-family: Arial, sans-serif;
    background-color: rgba(255, 255, 255, 0.5);
    margin: 0;
    padding: 0;
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
    }

    .container {
    padding: 20px;
    max-width: 600px;
    background-color: rgba(255, 255, 255, 0.8);
    border-radius: 10px;
    overflow: auto;
    }

    .valor-label {
    display: block;
    margin-bottom: 5px;
    }

    input[type='text'], input[type='number'] {
    width: calc(100% - 32px);
    padding: 10px;
    margin-bottom: 10px;
    }

    input[type='submit'] {
    background-color: #4CAF50;
    color: white;
    padding: 10px 15px;
    border: none;
    cursor: pointer;
    display: block;
    margin: 0 auto;
    margin-top: 20px;
    }

    input[type='submit']:hover {
    background-color: #45a049;
    }

    table {
    width: 100%;
    border-collapse: collapse;
    margin-top: 20px;
    }

    th, td {
    border: 1px solid #ddd;
    padding: 8px;
    text-align: left;
    }
    th {
    background-color: #f2f2f2;
    }

    </style>
    </head>
    <body>
    <div class="container">
    <h2 style="text-align: center;">Driveline SW & Calibration</h2>
    <form action='/save' method="post" onsubmit='return validateForm()'>
    <label class='valor-label' for='textValue'>Texto até 8 letras:</label>
    <input type='text' id='textValue' name='textValue' maxlength='8'><br><br>
    <label class='valor-label' for='brightness'>Brightness:</label>
    <input type='number' id='brightness' name='brightness' min='0' max='15'><br><br>
    <label class='valor-label'>Escolha a animação:</label>
    <select id='animationType' name='animationType'>
    <option value='0'>Walker</option>
    <option value='1'>Pacman</option>
    <option value='2'>Scroll</option>
    </select><br><br>
    <input type='submit' value='Send'>
    </form>

    <table>
    <thead>
    <tr>
    <th>Ordem</th>
    <th>Nome</th>
    </tr>
    </thead>
    <tbody>
    <tr>
    <td>1</td>
    <td>Jeff</td>
    </tr>
    <tr>
    <td>2</td>
    <td>Barcellos</td>
    </tr>
    <tr>
    <td>3</td>
    <td>Pablo</td>
    </tr>
    <tr>
    <td>3</td>
    <td>Celio</td>
    </tr>
    <tr>
    <td>4</td>
    <td>Heron</td>
    </tr>
    <tr>
    <td>5</td>
    <td>Alan</td>
    </tr>
    <tr>
    <td>6</td>
    <td>Velasques</td>
    </tr>
    <tr>
    <td>7</td>
    <td>Diego</td>
    </tr>
    <tr>
    <td>8</td>
    <td>Mellem</td>
    </tr>
    <tr>
    <td>9</td>
    <td>Jonathan</td>
    </tr>
    <tr>
    <td>10</td>
    <td>Wata</td>
    </tr>
    <tr>
    <td>11</td>
    <td>Gui</td>
    </tr>
    <!-- Adicione mais linhas conforme necessário -->
    </tbody>
    </table>

    </div>

    <script>
    function validateForm() {
    var animationOption = document.getElementById('animationOption');
    if (animationOption.checked) {
    var brightness = document.getElementById('brightness').value;
    var textValue = document.getElementById('textValue').value;
    textValue = textValue.slice(0, 8);
    document.getElementById('textValue').value = textValue;

    if (brightness === '') {
    document.getElementById('brightness').value = '0';
    }
    return true;
    }
    }
    </script>

    </body>
    </html>
    )";
  request->send(200, "text/html", message);
}

void handleData(AsyncWebServerRequest *request) {
  if (request->method() == HTTP_POST) 
  {
    animationType = request->arg("animationType").toInt();
    textInpt = request->arg("textValue");
    brightness = request->arg("brightness").toInt();

    EEPROM.put(ADDR_BRIGHTNESS, brightness);
    EEPROM.put(ADDR_ANIMATION_TYPE, animationType);
    for (int i = 0; i < prevLen; i++)
      EEPROM.write(ADDR_TEXT_VALUE + i, 0); 
    EEPROM.put(ADDR_TEXT_VALUE, textInpt);
    prevLen = textInpt.length();
    EEPROM.commit();
    // Send response to client
    request->send(200, "text/plain", "Data saved successfully");
    } 
  else{
    request->send(400, "text/plain", "Invalid request");
  }
    
}

void setup() {
  Serial.begin(115200);
  
  // Start WiFi access point mode
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(apIP, apIP, subnet); // Set the IP address and subnet mask for the access point
  // Set up route handlers
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      handleRoot(request);
  });
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
      handleData(request);
  });
  // Start the server
  server.begin();

  // Get access point IP
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP Address: ");
  Serial.println(IP);

  EEPROM.begin(512);
  EEPROM.get(ADDR_BRIGHTNESS, brightness);
  EEPROM.get(ADDR_ANIMATION_TYPE, animationType);
  EEPROM.get(ADDR_TEXT_VALUE, textInpt);

  // Sleep set up
  pinMode(TOUCH0_PIN, INPUT_PULLUP);
  //touchSleepWakeUpEnable(TOUCH1_PIN, TOUCH_THRESHOLD);
  //touchAttachInterrupt(TOUCH0_PIN, handleTouch, TOUCH_THRESHOLD);
  // timer15Min.start();

  // Display set up
  Display.begin();
  Display.setIntensity(brightness);
  Display.displayClear();
}

void loop() {
  //touchAttachInterrupt(TOUCH0_PIN, handleTouch, TOUCH_THRESHOLD);
  //timer15Min.update();
  Serial.print("Brilho:"); Serial.println(brightness);
  Serial.print("Animaçao:"); Serial.println(animationType);
  Serial.print("Texto:"); Serial.println(textInpt.c_str());
  delay(1000);

  // if (Display.displayAnimate()) { 
  //   Display.setIntensity(brightness);
  //   switch (animationType)
  //   {
  //     case 0:
  //       Display.setSpriteData(walker, W_WALKER, F_WALKER, walker, W_WALKER, F_WALKER);
  //       Display.displayText(textInpt.c_str(), PA_CENTER, scrollSpeed, pauseTime, PA_SPRITE, PA_SPRITE);
  //       break;
  //     case 1:
  //       Display.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2, F_PMAN2);
  //       Display.displayText(textInpt.c_str(), PA_CENTER, scrollSpeed, pauseTime, PA_SPRITE, PA_SPRITE);
  //       break;
  //     case 2:
  //       Display.displayText(textInpt.c_str(), PA_CENTER, scrollSpeed, pauseTime, PA_SCROLL_RIGHT);
  //       break;
  //   }
  //   Display.displayClear();
  // }

  // if (flagSleep) {
  //   flagSleep = false;
  //   Display.displayClear();
  //   touchDetachInterrupt(TOUCH0_PIN);
  //   esp_deep_sleep_start();
  // }
}