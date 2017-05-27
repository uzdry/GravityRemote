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

#define WEB_SERVER_BUFFER_SIZE 1024
#define WEB_SERVER_CLIENT_TIMEOUT 100

#define DEFAULT_SPEED 10.0
#ifdef DEBUG
#define BADGE_PULL_INTERVAL 60000
#else
#define BADGE_PULL_INTERVAL 5*60*1000
#endif

// Color definitions
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define DEFAULT_THEME "Light"
#define CONFIG_PREFIX "/deflt/conf/"

char ssid[] = "GPN-open";
char pw[] = "";

WiFiClient client;


Badge badge;
int readBuf[WEB_SERVER_BUFFER_SIZE];

bool autoTheme = false;
bool isDark = false;
unsigned char wifiLight = 255;
int wifi_hue = 0;
int wifi_hue_target = 0;


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
  packet.type = -1;
  packet.pid = -1;

  Serial.begin(115200);
  badge.init();

  badge.setBacklight(true);

  initBadge();
  bConnect();
  clearScreen();

  setupNumbers();

}

void loop(){
  float angle = setupNumbers();
  client.printf("v %.6f\n", DEFAULT_SPEED);
  client.printf("%.6f\n", angle);
  client.flush();
}

void loopl() {
  delay(1000);
  client.readBytes((char *) &packet, 8);
  switch (packet.type) {
    case 1:
      ownPID = packet.pid;
      Serial.printf("PlayerID: %d\n", packet.pid);
      tft.printf("Player: %d\n", packet.pid);
      break;
    case 2:
      Serial.printf("Player left: %d\n", packet.pid);
      tft.printf("Player left: %d\n", packet.pid);
      client.println("r ");
      break;
    case 3:
      client.readBytes((char *) &pPos, 8);
      Serial.printf("PlayerPos: %f %f\n", pPos.x, pPos.y);
      tft.printf("PlayerPos: %f %f", pPos.x, pPos.y);
      client.println("r ");
      break;

    case 4:
      int n;
      long temp;
      client.readBytes((char *) &n, 4);
      Serial.printf("Player finished: %d\n", n);
      tft.printf("Player finished: %d\n", n);

      for (int i = 0; i < n; i++) {
        client.readBytes((char *) &temp, 8);
      }

      if (packet.pid == ownPID) {
        client.println(createVString(0, 0));
      }

      break;
    case -1:
      break;
    default:
      client.println(createVString(0, 0));
      Serial.printf("Error: %d\n", packet.type);
      tft.print("OR ELSE");
      break;
  }
}



void bConnect() {
  WiFi.begin(ssid, pw);

  tft.print("Connecting");
  uint8_t cnt = 0;

  while (WiFi.status() != WL_CONNECTED) {
    tft.print(".");
    cnt++;
    if (cnt % 4 == 0) {
      tft.fillRect(63, 22, 30, 10, BLACK);
    }

    delay(250);
  }
  Serial.println("WiFi connected");
  tft.setTextColor(BLACK);
  tft.writeFramebuffer();
  tft.println("WiFi connected!");
  tft.println(WiFi.localIP());

  while (!client.connect("94.45.244.42", 3490)) {
    Serial.println("connection failed");
    tft.println("Connection failed!");
    delay(500);
  }
  Serial.println("Client connected");

  client.println("n ItSaMe");

  client.println("b");

  client.printf("v %.6f\n", DEFAULT_SPEED);
  client.printf("%.6f\n", 0);
}

float setupNumbers() {
  badge.setAnalogMUX(MUX_JOY);
  tft.setFont(&FreeSans9pt7b);
  tft.writeFramebuffer();
  tft.setTextColor(YELLOW);
  int adc;
  int8_t digit0 = 0, digit1 = 0, digit2 = 0, digit3 = 0, selDigit = 0, lastSelDigit;
  uint16_t lastRomID = 1337, romID;
  while(digitalRead(16) == HIGH);
  
  while (digitalRead(16) == LOW) {

    delay(10);
    adc = analogRead(A0);

    if (adc < UP + OFFSET && adc > UP - OFFSET) {
      if (selDigit == 0)
        digit0++;
      if (selDigit == 1)
        digit1++;
      if (selDigit == 2)
        digit2++;
      if (selDigit == 3)
        digit3++;
      delay(150);
    }

    else if (adc < DOWN + OFFSET && adc > DOWN - OFFSET) {
      if (selDigit == 0)
        digit0--;
      if (selDigit == 1)
        digit1--;
      if (selDigit == 2)
        digit2--;
      if (selDigit == 3)
        digit3--;
      delay(150);
    }

    else if (adc < LEFT + OFFSET && adc > LEFT - OFFSET) {
      selDigit--;
      delay(150);
    }

    else if (adc < RIGHT + OFFSET && adc > RIGHT - OFFSET) {
      selDigit++;
      delay(150);
    }

    if (digit0 > 9)
      digit0 = 0;

    if (digit0 < 0)
      digit0 = 9;

    if (digit1 > 9)
      digit1 = 0;

    if (digit1 < 0)
      digit1 = 9;

    if (digit2 > 9)
      digit2 = 0;

    if (digit2 < 0)
      digit2 = 9;

    if (digit3 > 9)
      digit3 = 0;

    if (digit3 < 0)
      digit3 = 9;

    if (selDigit > 3)
      selDigit = 0;

    if (selDigit < 0)
      selDigit = 3;

    romID = digit0 * 1000 + digit1 * 100 + digit2 * 10 + digit3;
    if (romID != lastRomID || selDigit != lastSelDigit) {
      tft.fillRect(44, 30, 50, 20, BLACK);
      tft.fillRect(45 + 10 * selDigit, 32, 10, 15, 0x738E);

      tft.setCursor(45, 45);
      tft.print(digit0);
      tft.print(digit1);
      tft.print(digit2);
      tft.print(digit3);
      tft.writeFramebuffer();
      lastRomID = romID;
      lastSelDigit = selDigit;
    }
  }

  return ((float) romID)/10;
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

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(YELLOW);
  tft.setCursor(2, 15);
  tft.println("Gravity...");
  tft.drawLine(2, 18, 126, 18, WHITE);
  tft.drawLine(2, 112, 126, 112, WHITE);

  tft.setFont();
  tft.setTextSize(1);
  tft.writeFramebuffer();

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
  tft.writeFramebuffer();
}

String createVString(float a, float b) {
  String values = "v ";
  values = values + String(a);
  values = values + "\n";
  values = values + String(b);
  return values;
}

