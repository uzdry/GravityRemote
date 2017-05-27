#include<GPNBadge.hpp>
#include <ESP8266WiFi.h>
#include <BadgeUI.h>
#include <UIThemes.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <gfxfont.h>
#include <Fonts/FreeSans9pt7b.h>

#include <TFT_ILI9163C.h>

#include "rboot.h"
#include "rboot-api.h"

// Server Defs
#define MSG_OWNID 1
#define MSG_PLAYERLEAVE 2
#define MSG_PLAYERPOS 3
#define MSG_SHOTFINISHED 4 /* not sent for bot protocol >= 6 */

#define MSG_SHOTBEGIN 5
#define MSG_SHOTFIN 6 /* replaces msg 4 */
#define MSG_GAMEMODE 7 /* not sent for bot protocol > 8 */
#define MSG_OWN_ENERGY 8


// Color definitions
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

//char ssid[] = "NewtonWars";
//char pw[] = "Gravity!";
//char ip[] = "192.168.8.8";
char ssid[] = "GPN-open";
char pw[] = "";
char ip[] = "94.45.244.42";
int port = 3490;


WiFiClient client;
Badge badge;

int8_t digits[11];
int8_t selDigit = 0, lastSelDigit;

float curDeg = 0;
float curVel = 10;
float lastDeg = curDeg;
float lastVel = curVel;


typedef struct {
  int type;
  int pid;
} Packet;

typedef struct {
  float x;
  float y;
} PlayerPos;


Packet packet;
PlayerPos pPos;
int ownPID;


void setup() {
  /// Variables
  memset(digits, 0, 11);
  digits[7] = 1;
  packet.type = -1;
  packet.pid = -1;

  Serial.begin(115200);
  badge.init();

  initBadge();
  bConnect();
  clearScreen();

}

void loop() {
  setupNumbers();

  if (curVel != lastVel) {
    client.printf("v %.6f\n", curVel);
    Serial.printf("Velocity %.6f\n", curVel);
    lastVel = curVel;
  }
  Serial.printf("Shooting %.6f\n", curDeg);
  client.printf("%.6f\n", curDeg);
  client.printf("u\n");

  if (client.available())
    handleIn();

  delay(300);


}

void handleIn() {

  client.readBytes((char *) &packet, 8);
  switch (packet.type) {
    case 1:
      ownPID = packet.pid;
      Serial.printf("PlayerID: %d\n", packet.pid);
      //      tft.printf("Player: %d\n", packet.pid);
      break;
    case 2:
      Serial.printf("Player left: %d\n", packet.pid);
      //      tft.printf("Player left: %d\n", packet.pid);
      //      client.println("r ");
      break;
    case 3:
      client.readBytes((char *) &pPos, 8);
      Serial.printf("PlayerPos: %f %f\n", pPos.x, pPos.y);
      //      tft.printf("PlayerPos: %f %f", pPos.x, pPos.y);
      //      client.println("r ");
      break;

    case 4:
      int n;
      long temp;
      client.readBytes((char *) &n, 4);
      Serial.printf("Player finished: %d\n", n);
      //      tft.printf("Player finished: %d\n", n);

      //      for (int i = 0; i < n; i++) {
      //        client.readBytes((char *) &temp, 8);
      //      }
      //
      //      if (packet.pid == ownPID) {
      //        client.println(createVString(0, 0));
      //      }

      break;
    case MSG_OWN_ENERGY:
      double value;
      client.readBytes((char *) &value, 8);
      showEnergy(value);
      break;

    case -1:
      break;
    default:
      Serial.printf("Error: %d\n", packet.type);
      break;
  }
}



void bConnect() {
  tft.println("Connecting to Wifi");
  WiFi.begin(ssid, pw);

  uint8_t cnt = 0;

  while (WiFi.status() != WL_CONNECTED) {
    cnt++;
    if (cnt % 4 == 0) {
      tft.print('.');

    }

    delay(250);
  }

  tft.println("\nWiFi connected!");
  tft.println(WiFi.localIP());

  tft.println("Connecting Server");
  while (!client.connect(ip, port)) {
    Serial.println("connection failed");
    tft.println("Connection failed!");
    delay(500);
  }
  Serial.println("Client connected");

  client.println("n ItSaMe");

  while (client.available()) {
    client.read();
    Serial.print('.');
  }
  Serial.println("");

}

void setupNumbers() {
  badge.setAnalogMUX(MUX_JOY);
  tft.setFont(&FreeSans9pt7b);

  tft.setTextColor(YELLOW);
  int adc;
  int16_t lastRomID = 1337, romID, velOc, lastVelOc = 1338;
  while (digitalRead(16) == HIGH);
  bool changed = true;
  while (digitalRead(16) == LOW) {
    delay(10);
    adc = analogRead(A0);


    if (adc < UP + OFFSET && adc > UP - OFFSET) {
      changed = true;
      digits[selDigit]++;
      delay(150);
    }
    else if (adc < DOWN + OFFSET && adc > DOWN - OFFSET) {
      changed = true;
      digits[selDigit]--;
      delay(150);
    }
    else if (adc < LEFT + OFFSET && adc > LEFT - OFFSET) {
      changed = true;
      selDigit--;
      delay(150);
    }
    else if (adc < RIGHT + OFFSET && adc > RIGHT - OFFSET) {
      changed = true;
      selDigit++;
      delay(150);
    }

    // Check for "out of bounds" values
    for (int i = 10; i >= 0; i--) {
      if (digits[i] > 9) {

        digits[i] = 0;

        if (i != 0 && i != 7)
          digits[i - 1]++;

      }

      else if (digits[i] < 0) {

        digits[i] = 9;

        if (i != 0 && i != 7)
          digits[i - 1]--;

      }
    }

    // First digit of first line may only be between 0 and 3
    if (digits[0] == 9) digits[0] = 3;
    if (digits[0] > 3) digits[0] = 0;
    if (digits[0] < 0) digits[0] = 3;

    // First digit of lower line may only be between 0 and 4
    if (digits[7] == 9) digits[7] = 4;
    if (digits[7] > 5) digits[7] = 0;
    if (digits[7] < 0) digits[7] = 4;

    if (selDigit > 10)
      selDigit = 0;

    if (selDigit < 0)
      selDigit = 10;

    if (changed) {
      tft.fillRect(35, 30, 93, 60, BLACK);

      if (selDigit > 6) {
        if (selDigit > 8)
          tft.fillRect(45 + 5 + 10 * (selDigit - 7), 62, 10, 15, 0x738E);
        else
          tft.fillRect(45 + 10 * (selDigit - 7), 62, 10, 15, 0x738E);
      }
      else
      {
        if (selDigit > 2)
          tft.fillRect(45 + 5 + 10 * selDigit, 32, 10, 15, 0x738E);
        else
          tft.fillRect(45 + 10 * selDigit, 32, 10, 15, 0x738E);
      }

      tft.setCursor(0, 45);
      tft.print("deg");
      tft.setCursor(0, 75);
      tft.print("vel");

      // UPPER
      tft.setCursor(45, 45);
      tft.print(digits[0]);
      tft.print(digits[1]);
      tft.print(digits[2]);
      tft.print('.');
      tft.print(digits[3]);
      tft.print(digits[4]);
      tft.print(digits[5]);
      tft.print(digits[6]);

      // LOWER
      tft.setCursor(45, 75);
      tft.print(digits[7]);
      tft.print(digits[8]);
      tft.print('.');
      tft.print(digits[9]);
      tft.print(digits[10]);



      lastVelOc = velOc;
      lastRomID = romID;
      lastSelDigit = selDigit;
    }

    changed = false;

  }

  curDeg = (digits[0] * 100) + (digits[1] * 10) + (digits[2] * 1) + (digits[3] * 0.1) + (digits[4] * 0.01) + (digits[5] * 0.001) + (digits[6] * 0.0001);
  curVel = (digits[7] * 10) + (digits[8] * 1) + (digits[9] * 0.1) + (digits[10] * 0.01);

}

void initBadge() { //initialize the badge

  badge.setBacklight(true);


  Serial.print("Battery Voltage:  ");
  Serial.println(badge.getBatVoltage());
  Serial.print("Light LDR Level:  ");
  Serial.println(badge.getLDRLvl());


  tft.begin(); //initialize the tft. This also sets up SPI to 80MHz Mode 0
  tft.setRotation(2);
  tft.scroll(32);

  tft.fillScreen(BLACK);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(YELLOW);
  tft.setCursor(2, 15);
  tft.println("Gravity...");
  tft.drawLine(2, 18, 126, 18, WHITE);
  tft.drawLine(2, 112, 126, 112, WHITE);

  tft.setFont();
  tft.setTextSize(1);

  badge.setBacklight(true);

}

void clearScreen() {
  tft.fillScreen(BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(YELLOW);
  tft.setCursor(2, 15);
  tft.println("Gravity...");
  tft.drawLine(2, 18, 126, 18, WHITE);
  tft.drawLine(2, 112, 126, 112, WHITE);

  tft.setFont();
  tft.setTextSize(1);

}

void showEnergy(double energy) {
  tft.fillRect(2, 116, 60, 10, BLACK);
  tft.setTextColor(YELLOW, BLACK);
  tft.setCursor(3, 117);
  tft.print(String(energy));
}

String createVString(float a, float b) {
  String values = "v ";
  values = values + String(a);
  values = values + "\n";
  values = values + String(b);
  return values;
}

