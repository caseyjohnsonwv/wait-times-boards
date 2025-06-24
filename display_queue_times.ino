#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

#define RGB565(r, g, b) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3) )
#define BLACK  0x0000
#define WHITE  0xFFFF
#define UNIVERSAL_BLUE RGB565(0, 110, 255)
#define SANDY_TAN RGB565(140, 127, 88)

const int maxDisplay = 4;
const int maxTotalRides = 18;
String rideNames[maxTotalRides];
String rideWaits[maxTotalRides];
int rideCount = 0;

int currentIndex = 0;
unsigned long lastCycleTime = 0;
const unsigned long cycleInterval = 7000;
bool needsInitialDisplayBoot = true;


void setup() {
  Serial.begin(9600);
  uint16_t ID = tft.readID();
  if (ID == 0xD3D3) ID = 0x9481;
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.setTextWrap(false);

  showHeaderFooter();
  showWaiting();
}


void loop() {
  handleSerial();

  if (needsInitialDisplayBoot) {
    tft.fillScreen(BLACK);
    showHeaderFooter();
    needsInitialDisplayBoot = false;
  }

  // for maxDisplay+1 or more rides
  if (rideCount > maxDisplay && millis() - lastCycleTime > cycleInterval) {
    lastCycleTime = millis();
    currentIndex = (currentIndex + maxDisplay) % rideCount;
    showRidePage();
  }
}


void showHeaderFooter() {
  // Header
  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, UNIVERSAL_BLUE);

  String headerText = "Islands of Adventure - Wait Times";
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(headerText, 0, 0, &x1, &y1, &w, &h);
  int xHeader = (tft.width() - w) / 2;
  tft.fillRect(0, 0, tft.width(), 40, UNIVERSAL_BLUE); // Optional: clear header area
  tft.setCursor(xHeader, 15);
  tft.print(headerText);

  // Footer
  tft.setTextColor(WHITE, SANDY_TAN);
  String footerText = "Powered by Queue-Times";
  tft.getTextBounds(footerText, 0, 0, &x1, &y1, &w, &h);
  int yFooter = tft.height() - h - 5;
  int xFooter = tft.width() - w - 10;
  tft.fillRect(0, yFooter - 5, tft.width(), h + 10, SANDY_TAN); // Optional: clear footer area
  tft.setCursor(xFooter, yFooter);
  tft.print(footerText);
}


void handleSerial() {
  if (Serial.available()) {
    String payload = Serial.readStringUntil('\n');
    payload.trim();

    if (payload.startsWith("[RESET]")) {
      rideCount = 0;
    }
    else if (payload.startsWith("R:")) {
      int delimiterIdx = payload.indexOf(';', 2);
      if (delimiterIdx > 2 && rideCount < maxTotalRides) {
        String name = payload.substring(2, delimiterIdx);
        String waitTime = payload.substring(delimiterIdx + 1);

        rideNames[rideCount] = name;
        rideWaits[rideCount] = waitTime;
        rideCount++;
      }
    }
  }
}


String truncateName(String name) {
  if (name.length() <= 28) {
    return name;
  }
  return name.substring(0, 25) + "...";
}


void showRideLine(String name, String waitTime, int lineNum) {
  int yStart = 80 + lineNum * 50;

  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);

  String displayName = truncateName(name);

  // Left-aligned truncated ride name
  tft.setCursor(10, yStart);
  tft.print(displayName);

  // Right-aligned wait time
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(waitTime, 0, 0, &x1, &y1, &w, &h);
  int xWait = tft.width() - w - 10;
  tft.setCursor(xWait, yStart);
  tft.print(waitTime);
}


void showRidePage() {
  tft.fillRect(0, 40, tft.width(), tft.height() - 80, BLACK); // Clear ride area (leaves header/footer)

  for (int i = 0; i < maxDisplay; i++) {
    int idx = (currentIndex + i) % rideCount;
    if (idx >= rideCount) break;

    showRideLine(rideNames[idx], rideWaits[idx], i);
  }
}


void showWaiting() {
  tft.fillScreen(BLACK);
  tft.setCursor(20, tft.height() / 2);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.setFont(NULL);
  tft.print("Awaiting data...");
}
