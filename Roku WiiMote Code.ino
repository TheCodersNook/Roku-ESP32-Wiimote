#include <WiFi.h>
#include <WiFiManager.h>
#include <math.h>
#include "ESP32Wiimote.h"
const char* ROKU_IP = "REPLACE_WITH_YOUR_ROKUS_IP";    // <-- PUT YOUR ROKU'S IP HERE
static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.nunchukStickThreshold = 5;
    config.txQueueSize = 32;
    config.rxQueueSize = 32;
    config.fastReconnectTtlMs = 24UL * 60UL * 60UL * 1000UL;  
    config.auth.serialPrivilegedToken = "esp32wiimote_serial_token_v1";
    config.auth.wifiApiToken = "esp32wiimote_wifi_api_token_v1";
    config.wifi.enabled = false;
    config.wifi.network = {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"};
    return config;
}
ESP32Wiimote wiimote(makeWiimoteConfig());
struct ButtonName {
  ButtonState bit;
  const char *name;
};
static const ButtonName kBridgeButtons[] = {
  {kButtonA, "A"},
  {kButtonOne, "1"},
  {kButtonTwo, "2"},
  {kButtonPlus, "+"},
  {kButtonMinus, "-"},
  {kButtonHome, "Home"},
  {kButtonUp, "Up"},
  {kButtonDown, "Down"},
  {kButtonLeft, "Left"},
  {kButtonRight, "Right"},
  {kButtonB, "B"},
};
static const size_t kBridgeButtonCount = sizeof(kBridgeButtons) / sizeof(kBridgeButtons[0]);
const unsigned long HOME_WAKE_MS = 2500;
const unsigned long HOME_HOLD_MS = 4500;
const unsigned long POWER_QUERY_MS = 60000;
const unsigned long POWER_MIN_CONNECT_MS = 1000;
const unsigned long RECONNECT_POWER_ON_MIN_MS = 10000;
const unsigned long REPEAT_START_MS = 450;
const unsigned long REPEAT_MS       = 250;
const unsigned long POINTER_ENTER_MS = 300;
const float POINT_DEADZONE_G = 0.05;  
const float POINT_CELL_X_G   = 0.13;  
const float POINT_CELL_Y_G   = 0.19;  
const int   POINT_MAX_CELL   = 6;     
const float POINT_HYST_G     = 0.03;  
const unsigned long POINT_KEY_MS = 200;  
const float POINT_MOTION_GATE_G = 0.15;  
const float POINT_RECAL_DEADZONE_G = 0.05; 
const unsigned long POINT_RECAL_MS = 2000; 
const float POINT_RECAL_SHIFT     = 0.25;  
const float EURO_MIN_CUTOFF = 2.0;   
const float EURO_BETA       = 0.05;  
const float EURO_D_CUTOFF   = 1.0;   
const int TILT_X_SIGN = +1;  
const int TILT_Y_SIGN = +1;  
const unsigned long BATTERY_READ_MS = 60000;
const int ESP_BOOT_PIN = 0;
const int LED_PIN = 2;
const unsigned long LED_BLINK_MS = 250;
const uint32_t BTN_LEFT  = static_cast<uint32_t>(kButtonLeft);
const uint32_t BTN_RIGHT = static_cast<uint32_t>(kButtonRight);
const uint32_t BTN_DOWN  = static_cast<uint32_t>(kButtonDown);
const uint32_t BTN_UP    = static_cast<uint32_t>(kButtonUp);
const uint32_t BTN_PLUS  = static_cast<uint32_t>(kButtonPlus);
const uint32_t BTN_MINUS = static_cast<uint32_t>(kButtonMinus);
const uint32_t BTN_HOME  = static_cast<uint32_t>(kButtonHome);
const uint32_t BTN_ONE   = static_cast<uint32_t>(kButtonOne);
const uint32_t BTN_TWO   = static_cast<uint32_t>(kButtonTwo);
const uint32_t BTN_A     = static_cast<uint32_t>(kButtonA);
const uint32_t BTN_B     = static_cast<uint32_t>(kButtonB);
const uint32_t BTN_ALL = BTN_LEFT | BTN_RIGHT | BTN_DOWN | BTN_UP | BTN_PLUS |
                         BTN_MINUS | BTN_HOME | BTN_ONE | BTN_TWO | BTN_A | BTN_B;
uint32_t prevBtn = 0;
static bool wasConnected = false;
static bool accelModeRequested = false;
static uint32_t lastPrintedMask = 0;
static bool havePrintedMask = false;
static unsigned long connectedAt = 0;  
unsigned long homePressedAt = 0;  
bool homeOnFired = false;         
bool homeOffFired = false;        
bool homeTurnedOn = false;        
bool oneHeld = false;
bool twoHeld = false;
bool chordFired = false;  
unsigned long bPressedAt = 0;  
bool bPointerPending = false;  
float centerSumX = 0, centerSumY = 0, centerSumZ = 0;
long centerCount = 0;
bool tvOn = true;  
unsigned long lastPowerQuery = 0;
unsigned long disconnectedAt = 0;  
bool supportsTvPower = false;
bool supportsVolume = false;
bool capsKnown = false;
bool lastSupportsTvPower = false;
bool lastSupportsVolume = false;
bool capsPrinted = false;
bool audioDeviceAvailable = false;
int audioVolume = -1;
bool audioMuted = false;
bool audioXmlPrinted = false;
bool volumeTestDone = false;  
bool volNotePrinted = false;  
int batteryLevel = 100;  
unsigned long lastBatteryRead = 0;
uint8_t lastBatteryMask = 0xFF;  
struct HoldButton {
  uint32_t mask;
  const char* key;
  unsigned long pressedAt;  
  unsigned long lastSent;
};
HoldButton repeaters[] = {
  { BTN_UP,     "Up",         0, 0 },
  { BTN_DOWN,   "Down",       0, 0 },
  { BTN_LEFT,   "Left",       0, 0 },
  { BTN_RIGHT,  "Right",      0, 0 },
  { BTN_PLUS,   "VolumeUp",   0, 0 },
  { BTN_MINUS,  "VolumeDown", 0, 0 },
};
const int NUM_REPEATERS = sizeof(repeaters) / sizeof(repeaters[0]);
bool pointerActive = false;
float restX = 128, restY = 128, restZ = 128;  
float restMag = 25.0;                          
bool verticalGrip = false;
int targetCol = 0, targetRow = 0;              
int curCol = 0, curRow = 0;                    
unsigned long pointerLastKey = 0;              
unsigned long lastPointerUpdate = 0;           
unsigned long deadzoneSince = 0;               
float filtX = 128, filtY = 128, filtZ = 128;   
struct OneEuro {
  float xFilt = 128;
  float dxFilt = 0;
  bool first = true;
};
OneEuro euro[3];
unsigned long lastAccelMicros = 0;  
WiFiManager wm;
unsigned long lastWifiCheck = 0;
bool wifiWasConnected = false;
bool wifiEverConnected = false;
bool bluetoothStarted = false;
bool ledState = false;
unsigned long lastLedToggle = 0;
int lastBootReading = HIGH;
int lastBootStable = HIGH;
unsigned long lastBootDebounce = 0;
static void printHeldMask(uint32_t mask) {
  const ButtonState buttons = buttonStateFromRaw(mask);
  bool printedAny = false;
  Serial.printf("0x%05x held:", mask);
  for (size_t i = 0; i < kBridgeButtonCount; ++i) {
    if (buttonStateHas(buttons, kBridgeButtons[i].bit)) {
      Serial.printf(" %s", kBridgeButtons[i].name);
      printedAny = true;
    }
  }
  if (!printedAny) {
    Serial.print(" none");
  }
  Serial.println();
}
int sendKey(const char* key) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi down, key dropped");
    return -1;
  }
  WiFiClient client;
  client.setTimeout(100);  
  if (!client.connect(ROKU_IP, 8060)) {  
    Serial.println("Roku connect failed");
    return -1;
  }
  client.print("POST /keypress/");
  client.print(key);
  client.print(" HTTP/1.1\r\nHost: ");
  client.print(ROKU_IP);
  client.print("\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
  String status = client.readStringUntil('\n');
  int code = -1;
  if (status.startsWith("HTTP/")) {
    code = status.substring(9, 12).toInt();
  }
  client.stop();
  Serial.print("-> ");
  Serial.print(key);
  Serial.print(" (HTTP ");
  Serial.print(code);
  Serial.println(")");
  bool isVol = (strcmp(key, "VolumeUp") == 0 || strcmp(key, "VolumeDown") == 0 ||
                strcmp(key, "VolumeMute") == 0);
  if (isVol && !volNotePrinted) {
    volNotePrinted = true;
    Serial.println("  (HTTP 200 only means the Roku got the request -- if CEC");
    Serial.println("   is off, the volume does not change. See the volume test.)");
  }
  return code;
}
void launchApp(const char* channelId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi down, launch dropped");
    return;
  }
  WiFiClient client;
  client.setTimeout(100);
  if (!client.connect(ROKU_IP, 8060)) {
    Serial.println("Roku connect failed");
    return;
  }
  client.print("POST /launch/");
  client.print(channelId);
  client.print(" HTTP/1.1\r\nHost: ");
  client.print(ROKU_IP);
  client.print("\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
  String status = client.readStringUntil('\n');
  int code = -1;
  if (status.startsWith("HTTP/")) {
    code = status.substring(9, 12).toInt();
  }
  client.stop();
  Serial.print("-> launch ");
  Serial.print(channelId);
  Serial.print(" (HTTP ");
  Serial.print(code);
  Serial.println(")");
}
bool queryTvPower() {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClient client;
  client.setTimeout(1000);
  if (!client.connect(ROKU_IP, 8060)) return false;
  client.print("GET /query/device-info HTTP/1.1\r\nHost: ");
  client.print(ROKU_IP);
  client.print("\r\nConnection: close\r\n\r\n");
  String body;
  body.reserve(2048);
  unsigned long start = millis();
  while ((client.connected() || client.available()) && millis() - start < 1000) {
    while (client.available()) {
      body += (char)client.read();
    }
    delay(1);
  }
  client.stop();
  int ver = body.indexOf("<software-version>");
  if (ver >= 0) {
    int verEnd = body.indexOf("</software-version>", ver);
    if (verEnd > ver) {
      String v = body.substring(ver + 18, verEnd);
      int dot1 = v.indexOf('.');
      int dot2 = v.indexOf('.', dot1 + 1);
      int major = (dot1 > 0) ? v.substring(0, dot1).toInt() : 0;
      int minor = (dot1 > 0 && dot2 > dot1) ? v.substring(dot1 + 1, dot2).toInt() : 0;
      bool avail = (major > 15) || (major == 15 && minor >= 3);
      if (avail != audioDeviceAvailable) {
        audioDeviceAvailable = avail;
        Serial.print("Roku OS: ");
        Serial.print(v);
        Serial.println(avail ? " (audio-device available)"
                             : " (audio-device needs OS 15.3+)");
      }
    }
  }
  int tvCap = body.indexOf("<supports-tv-power-control>");
  if (tvCap >= 0) {
    supportsTvPower = body.substring(tvCap, tvCap + 40).indexOf("true") >= 0;
    capsKnown = true;
  }
  int volCap = body.indexOf("<supports-audio-volume-control>");
  if (volCap >= 0) {
    supportsVolume = body.substring(volCap, volCap + 40).indexOf("true") >= 0;
    capsKnown = true;
  }
  if (capsKnown && (!capsPrinted || supportsTvPower != lastSupportsTvPower ||
                    supportsVolume != lastSupportsVolume)) {
    capsPrinted = true;
    lastSupportsTvPower = supportsTvPower;
    lastSupportsVolume = supportsVolume;
    Serial.print("TV power control (CEC): ");
    Serial.println(supportsTvPower ? "supported"
                                   : "NOT supported -- fix this: Roku: Settings > System > Control other devices (CEC) / check 1-touch play, TV Power, TV Volume / TV: turn on CEC (Samsung Anynet+, LG SIMPLINK, Sony BRAVIA Sync, Vizio CEC, Hisense CEC) / Then restart the Roku: Settings > System > Power > System restart");
    Serial.print("Volume control (CEC): ");
    Serial.println(supportsVolume ? "supported"
                                  : "NOT supported -- fix this: Roku: Settings > System > Control other devices (CEC) / check 1-touch play, TV Power, TV Volume / TV: turn on CEC (Samsung Anynet+, LG SIMPLINK, Sony BRAVIA Sync, Vizio CEC, Hisense CEC) / Then restart the Roku: Settings > System > Power > System restart");
  }
  if (body.indexOf("<power-mode>PowerOff</power-mode>") >= 0 ||
      body.indexOf("<power-mode>Ready</power-mode>") >= 0 ||
      body.indexOf("<power-mode>DisplayOff</power-mode>") >= 0) {
    tvOn = false;
    return true;
  }
  if (body.indexOf("<power-mode>PowerOn</power-mode>") >= 0) {
    tvOn = true;
    return true;
  }
  int pm = body.indexOf("<power-mode>");
  if (pm >= 0) {
    Serial.print("Unknown power-mode: ");
    Serial.println(body.substring(pm, pm + 40));
  }
  return false;  
}
bool queryAudioDevice() {
  if (!audioDeviceAvailable) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClient client;
  client.setTimeout(1000);
  if (!client.connect(ROKU_IP, 8060)) return false;
  client.print("GET /query/audio-device HTTP/1.1\r\nHost: ");
  client.print(ROKU_IP);
  client.print("\r\nConnection: close\r\n\r\n");
  String body;
  body.reserve(1024);
  unsigned long start = millis();
  while ((client.connected() || client.available()) && millis() - start < 1000) {
    while (client.available()) {
      body += (char)client.read();
    }
    delay(1);
  }
  client.stop();
  int vol = body.indexOf("<volume>");
  int mut = body.indexOf("<muted>");
  if (vol < 0 || mut < 0) return false;
  audioVolume = body.substring(vol + 8, body.indexOf("</volume>", vol)).toInt();
  audioMuted = body.substring(mut + 7, body.indexOf("</muted>", mut)) == "true";
  if (!audioXmlPrinted) {
    audioXmlPrinted = true;
    Serial.println("Audio-device reply (first read):");
    Serial.println(body);
  }
  return true;
}
void testVolume() {
  if (!audioDeviceAvailable) {
    Serial.println("Volume test skipped: Roku OS is below 15.3 (no audio-device).");
    Serial.println("Use the Volume control (CEC) verdict line instead.");
    return;
  }
  if (!queryAudioDevice()) {
    Serial.println("Volume test failed: could not read the audio state.");
    return;
  }
  int before = audioVolume;
  sendKey("VolumeUp");
  delay(500);  
  if (!queryAudioDevice()) {
    Serial.println("Volume test failed: could not re-read the audio state.");
    sendKey("VolumeDown");  
    return;
  }
  int after = audioVolume;
  sendKey("VolumeDown");  
  Serial.print("Volume test: ");
  Serial.print(before);
  Serial.print(" -> ");
  Serial.print(after);
  if (after > before) {
    Serial.println(" -- VOLUME WORKS");
  } else {
    Serial.println(" -- NO CHANGE. Volume keys are being ignored.");
    Serial.println("Fix: Roku: Settings > System > Control other devices (CEC) / check TV Volume / TV: turn on CEC / If audio goes through an optical soundbar, ECP volume cannot reach it -- use HDMI ARC or an IR blaster.");
  }
}
uint8_t batteryLedMask() {
  if (batteryLevel > 75) return 0b1111;
  if (batteryLevel > 50) return 0b0111;
  if (batteryLevel > 25) return 0b0011;
  return 0b0001;
}
void restoreLeds() {
  if (pointerActive) {
    lastBatteryMask = 0xFF;  
    wiimote.setLeds(0b1111);
  } else {
    lastBatteryMask = batteryLedMask();
    wiimote.setLeds(lastBatteryMask);
  }
}
void handleBatteryLeds(unsigned long now) {
  if (!ESP32Wiimote::isConnected()) return;
  if (now - lastBatteryRead >= BATTERY_READ_MS) {
    lastBatteryRead = now;
    ESP32Wiimote::requestBatteryUpdate();
  }
  int lvl = ESP32Wiimote::getBatteryLevel();
  if (lvl > 0 && lvl <= 100 && lvl != batteryLevel) {
    batteryLevel = lvl;
    Serial.print("Battery: ");
    Serial.print(batteryLevel);
    Serial.println("%");
  }
  if (pointerActive) return;
  uint8_t mask = batteryLedMask();
  if (mask != lastBatteryMask) {
    lastBatteryMask = mask;
    wiimote.setLeds(mask);
  }
}
float oneEuroAlpha(float dt, float cutoff) {
  float tau = 1.0f / (2.0f * PI * cutoff);
  return 1.0f / (1.0f + tau / dt);
}
float oneEuroUpdate(int axis, float x, float dt) {
  OneEuro &f = euro[axis];
  if (f.first) {
    f.first = false;
    f.xFilt = x;
    f.dxFilt = 0;
    return x;
  }
  float dx = (x - f.xFilt) / dt;
  float alphaD = oneEuroAlpha(dt, EURO_D_CUTOFF);
  f.dxFilt += alphaD * (dx - f.dxFilt);
  float cutoff = EURO_MIN_CUTOFF + EURO_BETA * fabs(f.dxFilt);
  float alpha = oneEuroAlpha(dt, cutoff);
  f.xFilt += alpha * (x - f.xFilt);
  return f.xFilt;
}
void oneEuroSnap(int axis, float x) {
  euro[axis].xFilt = x;
  euro[axis].dxFilt = 0;
  euro[axis].first = false;
}
void updateAccelFilter() {
  unsigned long nowUs = micros();
  float dt = 0.01f;  
  if (lastAccelMicros != 0) {
    dt = (nowUs - lastAccelMicros) / 1000000.0f;
    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.1f) dt = 0.1f;
  }
  lastAccelMicros = nowUs;
  AccelState a = wiimote.getAccelState();
  filtX = oneEuroUpdate(0, a.xAxis, dt);
  filtY = oneEuroUpdate(1, a.yAxis, dt);
  filtZ = oneEuroUpdate(2, a.zAxis, dt);
  if (bPointerPending && !pointerActive) {
    centerSumX += a.xAxis;
    centerSumY += a.yAxis;
    centerSumZ += a.zAxis;
    centerCount++;
  }
}
void enterPointerMode() {
  float cx, cy, cz;
  if (centerCount > 0) {
    cx = centerSumX / centerCount;
    cy = centerSumY / centerCount;
    cz = centerSumZ / centerCount;
  } else {
    AccelState a = wiimote.getAccelState();
    cx = a.xAxis; cy = a.yAxis; cz = a.zAxis;
  }
  oneEuroSnap(0, cx);
  oneEuroSnap(1, cy);
  oneEuroSnap(2, cz);
  filtX = cx;
  filtY = cy;
  filtZ = cz;
  restX = filtX;
  restY = filtY;
  restZ = filtZ;
  float dx = restX - 128.0f;
  float dy = restY - 128.0f;
  float dz = restZ - 128.0f;
  restMag = sqrt(dx * dx + dy * dy + dz * dz);
  if (restMag < 5.0f) restMag = 25.0f;  
  verticalGrip = (fabs(dy) > fabs(dz));
  targetCol = 0;
  targetRow = 0;
  curCol = 0;
  curRow = 0;
  pointerLastKey = 0;
  deadzoneSince = 0;
  lastPointerUpdate = millis();
  pointerActive = true;
  wiimote.setLeds(0b1111);  
  Serial.print("Pointer mode ON, center calibrated (restMag=");
  Serial.print(restMag);
  Serial.print(" counts, rest=(");
  Serial.print((int)restX);
  Serial.print(",");
  Serial.print(restY, 1);
  Serial.print(",");
  Serial.print(restZ, 1);
  Serial.print("), averaged over ");
  Serial.print(centerCount);
  Serial.print(" samples; ~25 = genuine-type accel, ~37 = this clone; grip=");
  Serial.println(verticalGrip ? "VERTICAL (B forward, up/down from Z)" : "FLAT (up/down from Y)");
}
void exitPointerMode() {
  pointerActive = false;
  targetCol = 0;
  targetRow = 0;
  curCol = 0;
  curRow = 0;
  deadzoneSince = 0;
  restoreLeds();
  Serial.println("Pointer mode OFF (highlight stays where it is)");
}
int hystAxis(float t, int cur, int target, float cellG) {
  if (target == cur) return cur;
  if (target > cur) {
    return (t > (cur + 0.5f) * cellG + POINT_HYST_G) ? target : cur;
  }
  return (t < (cur - 0.5f) * cellG - POINT_HYST_G) ? target : cur;
}
void updatePointer(unsigned long now) {
  AccelState raw = wiimote.getAccelState();
  float axg = (raw.xAxis - 128.0f) / restMag;
  float ayg = (raw.yAxis - 128.0f) / restMag;
  float azg = (raw.zAxis - 128.0f) / restMag;
  float aMag = sqrt(axg * axg + ayg * ayg + azg * azg);
  if (fabs(aMag - 1.0f) > POINT_MOTION_GATE_G) {
    deadzoneSince = 0;
    return;  
  }
  float tx = TILT_X_SIGN * (filtX - restX) / restMag;
  float ty = TILT_Y_SIGN * (verticalGrip ? (filtZ - restZ) : (filtY - restY)) / restMag;
  float ax = fabs(tx);
  float ay = fabs(ty);
  if (ax < POINT_RECAL_DEADZONE_G && ay < POINT_RECAL_DEADZONE_G) {
    if (deadzoneSince == 0) deadzoneSince = now;
  } else {
    deadzoneSince = 0;
  }
  float mag = sqrt(tx * tx + ty * ty);
  if (mag < POINT_DEADZONE_G) {
    targetCol = 0;
    targetRow = 0;
  } else {
    int newCol = (int)round(tx / POINT_CELL_X_G);
    int newRow = (int)round(ty / POINT_CELL_Y_G);
    if (newCol > POINT_MAX_CELL) newCol = POINT_MAX_CELL;
    if (newCol < -POINT_MAX_CELL) newCol = -POINT_MAX_CELL;
    if (newRow > POINT_MAX_CELL) newRow = POINT_MAX_CELL;
    if (newRow < -POINT_MAX_CELL) newRow = -POINT_MAX_CELL;
    targetCol = hystAxis(tx, targetCol, newCol, POINT_CELL_X_G);
    targetRow = hystAxis(ty, targetRow, newRow, POINT_CELL_Y_G);
  }
  lastPointerUpdate = now;
}
void drivePointer(unsigned long now) {
  if (!pointerActive) return;
  if (now - lastPointerUpdate > 500) return;
  int dCol = targetCol - curCol;
  int dRow = targetRow - curRow;
  if (dCol == 0 && dRow == 0) return;  
  if (now - pointerLastKey < POINT_KEY_MS) return;  
  const char* key;
  bool isCol;
  if (abs(dCol) >= abs(dRow)) {
    key = (dCol > 0) ? "Right" : "Left";
    isCol = true;
  } else {
    key = (dRow > 0) ? "Down" : "Up";
    isCol = false;
  }
  if (sendKey(key) == 200) {
    if (isCol) {
      curCol += (dCol > 0) ? 1 : -1;
    } else {
      curRow += (dRow > 0) ? 1 : -1;
    }
  }
  pointerLastKey = now;
}
void handlePointerRecal(unsigned long now) {
  if (!pointerActive) return;
  if (deadzoneSince == 0 || now - deadzoneSince < POINT_RECAL_MS) return;
  restX += POINT_RECAL_SHIFT * (filtX - restX);
  restY += POINT_RECAL_SHIFT * (filtY - restY);
  restZ += POINT_RECAL_SHIFT * (filtZ - restZ);
  float dx = restX - 128.0f;
  float dy = restY - 128.0f;
  float dz = restZ - 128.0f;
  restMag = sqrt(dx * dx + dy * dy + dz * dz);
  if (restMag < 5.0f) restMag = 25.0f;
  deadzoneSince = now;  
  Serial.println("Pointer center re-calibrated");
}
void handleButtons(uint32_t btn, unsigned long now) {
  uint32_t pressed  = btn & ~prevBtn;
  uint32_t released = prevBtn & ~btn;
  for (int i = 0; i < NUM_REPEATERS; i++) {
    if (pressed & repeaters[i].mask) {
      sendKey(repeaters[i].key);
      repeaters[i].pressedAt = now;
      repeaters[i].lastSent = now;
    }
    if (released & repeaters[i].mask) {
      repeaters[i].pressedAt = 0;
    }
  }
  if (pressed & BTN_A) {
    sendKey("Select");
  }
  if (pressed & BTN_HOME) {
    homePressedAt = now;
    homeOnFired = false;
    homeOffFired = false;
    homeTurnedOn = false;
  }
  if (released & BTN_HOME) {
    if (!homeOnFired && !homeOffFired) sendKey("Home");
    homePressedAt = 0;
  }
  if (pressed & BTN_ONE) {
    oneHeld = true;
    if (twoHeld && !chordFired) {
      sendKey("VolumeMute");
      chordFired = true;
    }
  }
  if (released & BTN_ONE) {
    oneHeld = false;
    if (!chordFired) launchApp("12");  
    if (!twoHeld) chordFired = false;
  }
  if (pressed & BTN_TWO) {
    twoHeld = true;
    if (oneHeld && !chordFired) {
      sendKey("VolumeMute");
      chordFired = true;
    }
  }
  if (released & BTN_TWO) {
    twoHeld = false;
    if (!chordFired) launchApp("291097");  
    if (!oneHeld) chordFired = false;
  }
  if (pressed & BTN_B) {
    bPressedAt = now;
    bPointerPending = true;
    centerSumX = 0; centerSumY = 0; centerSumZ = 0; centerCount = 0;
  }
  if (released & BTN_B) {
    if (bPointerPending) {
      sendKey("Back");
    } else if (pointerActive) {
      exitPointerMode();
    }
    bPointerPending = false;
    bPressedAt = 0;
  }
  prevBtn = btn;
}
void handleHolds(unsigned long now) {
  if (bPointerPending && bPressedAt != 0 && !pointerActive &&
      now - bPressedAt >= POINTER_ENTER_MS) {
    bPointerPending = false;
    enterPointerMode();
  }
  if (homePressedAt != 0 && !homeOnFired &&
      now - homePressedAt >= HOME_WAKE_MS) {
    homeOnFired = true;
    if (!tvOn) {
      if (sendKey("PowerOn") == 200) {
        tvOn = true;
        homeTurnedOn = true;  
      } else {
        Serial.println("PowerOn did not land (not HTTP 200)");
      }
    } else {
      Serial.println("TV already on, wake skipped");
    }
  }
  if (homePressedAt != 0 && !homeOffFired && !homeTurnedOn &&
      now - homePressedAt >= HOME_HOLD_MS) {
    homeOffFired = true;
    if (tvOn) {
      if (sendKey("PowerOff") == 200) {
        tvOn = false;
      } else {
        Serial.println("PowerOff did not land (not HTTP 200)");
      }
    } else {
      Serial.println("TV already off, power-off skipped");
    }
  }
  for (int i = 0; i < NUM_REPEATERS; i++) {
    if (repeaters[i].pressedAt != 0 &&
        now - repeaters[i].pressedAt >= REPEAT_START_MS &&
        now - repeaters[i].lastSent >= REPEAT_MS) {
      sendKey(repeaters[i].key);
      repeaters[i].lastSent = now;
    }
  }
  drivePointer(now);
}
void clearHeldState() {
  homePressedAt = 0;
  oneHeld = false;
  twoHeld = false;
  chordFired = false;
  bPressedAt = 0;
  bPointerPending = false;
  for (int i = 0; i < NUM_REPEATERS; i++) repeaters[i].pressedAt = 0;
  pointerActive = false;
  targetCol = 0;
  targetRow = 0;
  curCol = 0;
  curRow = 0;
  deadzoneSince = 0;
}
void startWifi() {
  wm.setConfigPortalBlocking(false);  
  wm.setConnectTimeout(10);           
  wm.setAPCallback([](WiFiManager* mgr) {
    Serial.println();
    Serial.println("Wi-Fi not set up yet.");
    Serial.println("Join the Wi-Fi network \"WiiRokuBridge\" on your");
    Serial.println("phone, then pick your home Wi-Fi and enter its");
    Serial.println("password on the setup page that pops up.");
  });
  Serial.println("Connecting to Wi-Fi (in the background)...");
  wm.autoConnect("WiiRokuBridge");  
}
bool startBluetooth();
void wifiWatchdog(unsigned long now) {
  bool wifiUp = (WiFi.status() == WL_CONNECTED);
  if (wifiUp && !wifiWasConnected) {
    Serial.print("Wi-Fi connected, ESP32 IP: ");
    Serial.println(WiFi.localIP());
    wifiEverConnected = true;
    if (queryTvPower()) {
      Serial.print("Roku reachable, TV is ");
      Serial.println(tvOn ? "on" : "off");
      lastPowerQuery = now;
      if (!volumeTestDone) {
        volumeTestDone = true;
        testVolume();
      }
    } else {
      Serial.println("Roku NOT reachable -- check the Roku IP and that");
      Serial.println("Control by mobile apps is Enabled on the Roku.");
    }
    if (!bluetoothStarted) {
      startBluetooth();
    }
  }
  wifiWasConnected = wifiUp;
  if (now - lastWifiCheck < 10000) return;
  lastWifiCheck = now;
  if (wifiEverConnected && !wifiUp) {
    Serial.println("Wi-Fi lost, reconnecting...");
    WiFi.disconnect();
    WiFi.begin();  
  }
}
bool startBluetooth() {
  Serial.println("Starting Bluetooth...");
  if (!wiimote.init()) {
    Serial.println("Bluetooth init failed -- will retry.");
    return false;
  }
  if (wiimote.getConfig().wifi.enabled) {
    wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);
  }
  wiimote.setAutoReconnectEnabled(true);
  tinyWiimoteSetFastReconnectTtlMs(24UL * 60UL * 60UL * 1000UL);
  bluetoothStarted = true;
  Serial.println("Bluetooth up. Press 1 + 2, then hold B and tilt to move.");
  return true;
}
void updateWaitingLed(unsigned long now) {
  if (now - lastLedToggle >= 500) {
    lastLedToggle = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}
void updatePairLed(unsigned long now) {
  if (ESP32Wiimote::isConnected()) {
    if (!ledState) {
      ledState = true;
      digitalWrite(LED_PIN, HIGH);
    }
    return;
  }
  if (now - lastLedToggle >= LED_BLINK_MS) {
    lastLedToggle = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}
void handleBootButton(unsigned long now) {
  int reading = digitalRead(ESP_BOOT_PIN);
  if (reading != lastBootReading) {
    lastBootDebounce = now;
    lastBootReading = reading;
  }
  if (now - lastBootDebounce > 50 && reading != lastBootStable) {
    lastBootStable = reading;
    if (reading == LOW) {  
      if (!ESP32Wiimote::isConnected()) {
        Serial.println("BOOT: no remote connected, kicking off discovery");
        wiimote.startDiscovery();
      } else {
        Serial.println("--- Status ---");
        Serial.print("Wi-Fi: ");
        if (WiFi.status() == WL_CONNECTED) {
          Serial.print("connected, ESP32 IP ");
          Serial.println(WiFi.localIP());
        } else {
          Serial.println("not connected");
        }
        Serial.print("Roku: ");
        Serial.println(ROKU_IP);
        Serial.print("Remote: connected, battery ");
        Serial.print(batteryLevel);
        Serial.println("%");
        Serial.print("TV: ");
        Serial.println(tvOn ? "on (last known)" : "off (last known)");
        Serial.println("--------------");
        testVolume();
      }
    }
  }
}
void setup() {
    Serial.begin(115200);
    delay(200);
    wiimoteSetLogLevel(2);
    Serial.println("Wii -> Roku bridge (built on SensorReadout)");
    pinMode(ESP_BOOT_PIN, INPUT_PULLUP);  
    pinMode(LED_PIN, OUTPUT);             
    digitalWrite(LED_PIN, LOW);
    startWifi();
    lastPowerQuery = millis();
    Serial.println("Set up Wi-Fi first -- Bluetooth starts once it connects.");
}
void loop() {
    wm.process();  
    unsigned long now = millis();
    wifiWatchdog(now);  
    if (!bluetoothStarted) {
      updateWaitingLed(now);
      static unsigned long lastBtRetry = 0;
      if (WiFi.status() == WL_CONNECTED && now - lastBtRetry >= 5000) {
        lastBtRetry = now;
        startBluetooth();  
      }
      delay(5);
      return;
    }
    wiimote.task();
    const bool connected = ESP32Wiimote::isConnected();
    if (connected != wasConnected) {
        if (connected) {
            bool firstConnect = (disconnectedAt == 0);
            unsigned long goneFor = firstConnect ? 0 : now - disconnectedAt;
            Serial.println("CONNECTED");
            connectedAt = now;
            disconnectedAt = 0;
            accelModeRequested = false;
            prevBtn = 0;
            lastBatteryRead = 0;  
            restoreLeds();  
            if (!tvOn) {
                if (firstConnect || goneFor >= RECONNECT_POWER_ON_MIN_MS) {
                    Serial.println("Remote reconnected -- turning the TV on");
                    if (sendKey("PowerOn") == 200) tvOn = true;
                } else {
                    Serial.println("Remote bounced back too fast -- PowerOn skipped");
                }
            }
        } else {
            Serial.println("DISCONNECTED");
            accelModeRequested = false;
            lastPrintedMask = 0;
            havePrintedMask = false;
            disconnectedAt = now;
            clearHeldState();
            if (tvOn && connectedAt != 0 && now - connectedAt >= POWER_MIN_CONNECT_MS) {
                Serial.println("Remote disconnected -- turning the TV off");
                if (sendKey("PowerOff") == 200) {
                    tvOn = false;
                } else {
                    Serial.println("PowerOff status unknown (no retry -- one send per disconnect)");
                }
            }
            connectedAt = 0;
        }
        wasConnected = connected;
    }
    if (connected && !accelModeRequested) {
        wiimote.setAccelerometerEnabled(true);
        if (wiimote.setReportingMode(ReportingMode::CoreButtonsAccel, true)) {
            Serial.println("Reporting mode set: CoreButtonsAccel continuous");
            accelModeRequested = true;
        }
    }
    if (connected && accelModeRequested) {
      updateAccelFilter();
    }
    while (wiimote.available() > 0) {
        const uint32_t mask = static_cast<uint32_t>(wiimote.getButtonState()) & BTN_ALL;
        if (!havePrintedMask || mask != lastPrintedMask) {
            printHeldMask(mask);
            lastPrintedMask = mask;
            havePrintedMask = true;
        }
        handleButtons(mask, now);
        if (pointerActive) updatePointer(now);
    }
    handleHolds(now);
    handlePointerRecal(now);
    handleBatteryLeds(now);
    handleBootButton(now);
    updatePairLed(now);
    if (now - lastPowerQuery >= POWER_QUERY_MS) {
      lastPowerQuery = now;
      queryTvPower();
    }
    delay(5);
}
