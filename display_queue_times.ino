#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

#define RGB565(r, g, b) ( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3) )
#define BLACK  0x0000
#define WHITE  0xFFFF
#define SANDY_TAN RGB565(140, 127, 88)

String parkName = "";
bool needsInitialDisplayBoot = true;
const int maxDisplay = 4;
int lineNum = 0;


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

  showWaiting();
}


void loop() {
  handleSerial();

  if (needsInitialDisplayBoot) {
    tft.fillScreen(BLACK);
    showHeaderFooter();
    needsInitialDisplayBoot = false;
  }
}


void showHeaderFooter() {
  // Header
  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, SANDY_TAN);

  String headerText = parkName + " - Wait Times";
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(headerText, 0, 0, &x1, &y1, &w, &h);
  int xHeader = (tft.width() - w) / 2;
  tft.fillRect(0, 0, tft.width(), 40, SANDY_TAN); // Optional: clear header area
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

    if (payload.startsWith("[UPDATE]")) {
      // Clear ride area (leaves header/footer)
      lineNum = 0;
      tft.fillRect(0, 40, tft.width(), tft.height() - 80, BLACK);
    }
    else if (payload.startsWith("R:")) {
      int delimiterIdx = payload.indexOf(';', 2);
      String name = payload.substring(2, delimiterIdx);
      String waitTime = payload.substring(delimiterIdx + 1);
      showRideLine(name, waitTime, lineNum);
      lineNum++;
    }
    else if (payload.startsWith("P:")) {
      parkName = payload.substring(2);
      parkName.trim();
      needsInitialDisplayBoot = true;
    }
  }
}


void showRideLine(String name, String waitTime, int lineNum) {
  int yStart = 80 + lineNum * 50;

  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);

  // Left-aligned truncated ride name
  tft.setCursor(10, yStart);
  tft.print(name);

  // Right-aligned wait time
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(waitTime, 0, 0, &x1, &y1, &w, &h);
  int xWait = tft.width() - w - 10;
  tft.setCursor(xWait, yStart);
  tft.print(waitTime);
}


void showWaiting() {
  tft.fillScreen(BLACK);
  tft.setCursor(20, tft.height() / 2);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(2);
  tft.setFont(NULL);
  tft.print("Booting...");

  bool connected = false;
  while (!connected) {
    if (Serial.available()) {
      String payload = Serial.readStringUntil('\n');
      if (payload.startsWith("[CONNECTED]")) {
        String ip = payload.substring(12);
        String resetMessageStr = "Reset: http://" + ip + "/reset";
        connected = true;
        tft.fillScreen(BLACK);
        tft.setCursor(20, tft.height() / 2);
        tft.print(resetMessageStr.c_str());
      }
      else if (payload.startsWith("[AP]")) {
        tft.fillScreen(BLACK);
        tft.setCursor(20, tft.height() / 2);
        tft.print("Awaiting WiFi configuration...");
      }
      else if (payload.startsWith("[ERROR]")) {
        tft.fillScreen(BLACK);
        tft.setCursor(20, tft.height() / 2);
        tft.print("Error connecting to WiFi");
      }
    }
  }

  delay(3000);
  tft.fillScreen(BLACK);
  Serial.println("[READY]");
  delay(1000);
}
