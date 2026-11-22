#include "ESP32Wiimote.h"

#include <Arduino.h>

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this sensor-focused example.
static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.nunchukStickThreshold = 5;
    config.txQueueSize = 32;
    config.rxQueueSize = 32;
    config.fastReconnectTtlMs = 180000;
    config.auth.serialPrivilegedToken = "esp32wiimote_serial_token_v1";
    config.auth.wifiApiToken = "esp32wiimote_wifi_api_token_v1";
    config.wifi.enabled = false;
    config.wifi.network = {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"};
    return config;
}

ESP32Wiimote wiimote(makeWiimoteConfig());

static bool wasConnected = false;
static bool accelModeRequested = false;

void setup() {
    Serial.begin(115200);
    delay(200);

    // INFO level keeps connection messages without HCI byte floods.
    wiimoteSetLogLevel(2);

    Serial.println("SensorReadout: initializing");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (wiimote.getConfig().wifi.enabled) {
        wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);
    }

    wiimote.setAutoReconnectEnabled(true);

    Serial.println("Ready. Press 1 + 2, then hold A + B for sensor readout.");
}

void loop() {
    wiimote.task();

    const bool connected = ESP32Wiimote::isConnected();
    if (connected != wasConnected) {
        if (connected) {
            Serial.println("CONNECTED");
            accelModeRequested = false;
        } else {
            Serial.println("DISCONNECTED");
            accelModeRequested = false;
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

    while (wiimote.available() > 0) {
        const ButtonState buttons = wiimote.getButtonState();
        const bool kReadoutHeld =
            buttonStateHas(buttons, kButtonA) && buttonStateHas(buttons, kButtonB);
        if (!kReadoutHeld) {
            continue;
        }

        AccelState accel = wiimote.getAccelState();
        NunchukState nunchuk = wiimote.getNunchukState();

        Serial.printf("wiimote accel: %3u/%3u/%3u", accel.xAxis, accel.yAxis, accel.zAxis);
        Serial.printf(" | nunchuk accel: %3u/%3u/%3u", nunchuk.xAxis, nunchuk.yAxis, nunchuk.zAxis);
        Serial.printf(" | nunchuk stick: %3u/%3u\n", nunchuk.xStick, nunchuk.yStick);

    }

    delay(10);
}
