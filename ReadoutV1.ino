#include "ESP32Wiimote.h"

#include <Arduino.h>


static ESP32WiimoteConfig makeWiimoteConfig() {
ESP32WiimoteConfig config;
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

static bool wasConnected = false;
static bool reportingModeRequested = false;
static uint32_t lastPrintedMask = 0;
static bool havePrintedMask = false;

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

void setup() {
Serial.begin(115200);
delay(200);

// INFO level keeps connection/disconnect messages without HCI byte floods.
wiimoteSetLogLevel(2);

Serial.println("CloneButtonBridge: initializing...");
if (!wiimote.init()) {
Serial.println("Init failed. Halting.");
while (true) {
delay(1000);
}
}

wiimote.setAutoReconnectEnabled(true);

Serial.println("Ready. Press 1 + 2 on the Wiimote.");
Serial.println("Bridge buttons: A 1 2 + - Home Up Down Left Right B");
Serial.println("Power is disconnect-only and is never reported as a button.");
}

void loop() {
wiimote.task();

const bool connected = ESP32Wiimote::isConnected();
if (connected != wasConnected) {
if (connected) {
Serial.println("CONNECTED");
reportingModeRequested = false;
} else {
Serial.println("DISCONNECTED");
reportingModeRequested = false;
lastPrintedMask = 0;
havePrintedMask = false;
}
wasConnected = connected;
}

if (connected && !reportingModeRequested) {
if (wiimote.setReportingMode(ReportingMode::CoreButtons, true)) {
Serial.println("Reporting mode set: CoreButtons continuous");
reportingModeRequested = true;
}
}

while (wiimote.available() > 0) {
const uint32_t mask = static_cast<uint32_t>(wiimote.getButtonState());
if (!havePrintedMask || mask != lastPrintedMask) {
printHeldMask(mask);
lastPrintedMask = mask;
havePrintedMask = true;
}
}

delay(5);
}
