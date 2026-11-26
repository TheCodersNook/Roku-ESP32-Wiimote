# Roku ESP32 Wiimote

**Use any Nintendo Wii Remote — even a clone — as a full remote for your Roku.**

Arrows, Select, Back, Home, volume, app shortcuts, TV power over HDMI-CEC, and a motion pointer: hold **B** and tilt the remote, and the highlight on your TV follows your hand.

```
Wii Remote --Bluetooth Classic--> ESP32 --Wi-Fi / Roku ECP--> Roku --HDMI-CEC--> TV
```

No extra hardware on the TV side. The ESP32 sits between the remote and the Roku and translates button presses and tilt into Roku External Control Protocol (ECP) commands over your home Wi-Fi.

---

## Features

- **Full button map** — D-pad arrows (hold to repeat), A = Select, B tap = Back, Home tap = Home, + / − = volume (hold to repeat)
- **App shortcuts** — 1 launches Netflix, 2 launches Disney+, 1+2 together = Mute
- **TV power** — Home hold 2.5 s turns the TV on, hold 4.5 s turns it off (via Roku PowerOn/PowerOff over HDMI-CEC). Turning the remote off also turns the TV off.
- **Motion pointer** — hold B for 300 ms, then point: the highlight moves where you tilt. Release B and it stays; tap A to select. Position control, not velocity — tilt right and the highlight goes right and *stays* there.
- **Battery LEDs** — the remote's four LEDs show battery in quarters (4 = >75%, 3 = >50%, 2 = >25%, 1 = low)
- **Reconnects on Power** — after a one-time SYNC pair, pressing Power on the remote reconnects it in about a second, no button-holding
- **Wi-Fi setup portal** — no Wi-Fi password in the code. First boot creates a `WiiRokuBridge` network; join it on your phone, pick your home Wi-Fi, done.
- **Closed-loop volume test** — on first connect the bridge asks the Roku whether volume actually changed and prints the verdict to Serial, so you know if your TV's CEC volume works

---

## Hardware requirements

| Item | Notes |
|---|---|
| ESP32 board (classic, **not** S3/C3/C6) | Any classic ESP32 with Bluetooth Classic — e.g. ESP32 DevKit / ESP32-D0WD-V3. **ESP32-S3 boards are BLE-only and cannot pair to a Wiimote.** |
| Wii Remote | Genuine Nintendo or a clone (tested with a ChamZA clone). Needs working Bluetooth. |
| Roku | Any Roku with External Control enabled (tested on Roku Ultra). |
| TV | Any TV for Roku control. HDMI-CEC (e.g. LG SIMPLINK) is only needed for TV power and volume. |
| USB cable + computer | For flashing the ESP32. |

No wiring, no soldering, no extra parts. The ESP32 talks to the remote over Bluetooth and to the Roku over Wi-Fi.

---

## Software requirements

- **Arduino IDE 2.x**
- **ESP32 board package, core 3.3.12** (installed through the IDE's Boards Manager)
- **ESP32Wiimote library — patched version 2.0.3**, included in this repo as `Fixed ESP32 Wiimote Package.zip`
- **WiFiManager by tzapu** (installed through the IDE's Library Manager)

### Why the patched library matters

The stock library from Library Manager does **not** work for this project: it has no handler for incoming Bluetooth connections, so a remote that is powered on never reconnects on its own. The patched version adds a listen-first reconnect (the ESP32 sits in page scan and accepts the remote's incoming page) plus NVS persistence of the paired remote.

**Rule of thumb:** install the library once from the ZIP in this repo, and **never update it from Library Manager** — updating replaces it with the stock version and breaks Power-on reconnect.

---

## Setting up Arduino IDE (step by step)

Do this once. Every step is a click or two; nothing here is typed into a terminal.

### Step 1 — Install Arduino IDE

Download and install **Arduino IDE 2.x** from <https://www.arduino.cc/en/software>. Open it once so it finishes first-run setup.

### Step 2 — Install ESP32 board support

1. Open **Tools → Board → Boards Manager** (or the Boards Manager icon in the left sidebar).
2. Search for **esp32**.
3. Install **esp32 by Espressif Systems**, version **3.3.12**. This download is large (hundreds of MB) and takes a few minutes.

If `esp32` does not appear in the search results: open **Arduino IDE → Settings** (macOS: **Arduino IDE → Preferences**), find **Additional boards manager URLs**, and paste this in, then reopen Boards Manager and try again:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

### Step 3 — Install WiFiManager by tzapu

1. Open **Sketch → Include Library → Manage Libraries** (or the Library Manager icon).
2. Search for **WiFiManager**.
3. Install **WiFiManager by tzapu** (the one with the author shown as tzapu / Benoit Blanchon's library entry — if several appear, pick the tzapu one).

If you skip this, the sketch will either fail to compile or fail on `#include <WiFiManager.h>`.

### Step 4 — Install the patched ESP32Wiimote library from the ZIP

This is the step people get wrong, so do it exactly:

1. **Delete any existing ESP32Wiimote library first.** In **Sketch → Include Library → Manage Libraries**, search for `ESP32Wiimote` and uninstall/remove it. (Alternatively delete the `ESP32Wiimote` folder from your Arduino libraries folder: `Documents/Arduino/libraries` on Windows/Linux, `~/Documents/Arduino/libraries` on macOS.)
2. In Arduino IDE choose **Sketch → Include Library → Add .ZIP Library…**
3. Select **`Fixed ESP32 Wiimote Package.zip`** from this repo (download the repo first if you haven't — see Step 5).
4. Wait for the “Library added to your libraries” message in the status bar.
5. **Do not** update this library from Library Manager later, and do not accept any “update available” prompt for it.

To confirm it worked, **Sketch → Include Library** should now list **ESP32Wiimote** under *Contributed libraries*.

### Step 5 — Open the sketch

1. On this repo's GitHub page click the green **Code** button → **Download ZIP**, then unzip it. (Or `git clone` the repo if you prefer.)
2. Double-click **`Roku WiiMote Code.ino`** — it opens in Arduino IDE.

> Arduino requires the `.ino` file to live in a folder with the *exact same name*. If the IDE offers to “move the file into a folder” or complains about the sketch name, say yes / put it in a folder called `Roku WiiMote Code` and open it from there.

### Step 6 — Put in your Roku's IP

In the sketch, find the line near the top:

```cpp
const char* ROKU_IP = "REPLACE_WITH_YOUR_ROKUS_IP";    // <-- PUT YOUR ROKU'S IP HERE
```

Replace `REPLACE_WITH_YOUR_ROKUS_IP` with the IP from **Settings → Network → About** on the Roku, keeping the quotes, e.g. `"192.168.1.100"`. That is the only line you need to change.

### Step 7 — Select the board and the port

1. Plug the ESP32 into your computer with a USB **data** cable (some cables are charge-only and will never show a port).
2. **Tools → Board → esp32 → ESP32 Dev Module**.
3. **Tools → Port** → pick the port that appeared when you plugged the board in (e.g. `COM5` on Windows, `/dev/cu.usbserial-…` or `/dev/cu.wchusbserial…` on macOS).

No port in the list? That is almost always the USB-serial driver. Most ESP32 DevKits use a **CP2102** or **CH340** chip — install the matching driver (Silicon Labs CP210x or WCH CH340), unplug, replug, and check **Tools → Port** again.

### Step 8 — Upload

1. **Tools → Upload Speed → 115200**. (Do not use 921600 — it fails on many boards.)
2. Click the **→ Upload** button (top left).
3. Wait for `Writing at 0x…` progress bars and finally `Hash of data verified.` / `Done uploading.`

If the IDE sits on `Connecting........_____` forever, hold the **BOOT** button on the ESP32 while the upload starts and release it once the progress bars appear. Some boards need this every time.

### Step 9 — Watch it run in the Serial Monitor

Open **Tools → Serial Monitor** and set the baud rate (dropdown, bottom right) to **115200** — that is what the sketch uses. You should see it connect to Wi-Fi, then `Bluetooth up`.

Keep the Serial Monitor open for the first connect: the bridge prints the Roku reachability check, the CEC verdict and the volume test verdict there. Nothing needs to be typed in — it is output only.

Once flashed, the ESP32 runs on its own: it only needs USB power (a phone charger works). You can close Arduino IDE.

---

## Setup

### 1. Roku settings

On the Roku:

1. **Settings → System → Advanced system settings → Control by mobile apps → Enabled** (or Permissive). Without this the ESP32 cannot send any keypresses.
2. **Settings → Network → About** — write down the **IP address**. You will put it in the sketch (Arduino IDE Step 6 above).

### 2. Flash the ESP32

Already covered in detail in **Setting up Arduino IDE** above. The short version:

- Board: **ESP32 Dev Module**
- Upload speed: **115200**
- Upload `Roku WiiMote Code.ino`
- Serial Monitor at **115200** to watch the logs

### 3. Connect Wi-Fi (first boot only)

On first boot the ESP32 creates a Wi-Fi network called **`WiiRokuBridge`**. Join it on your phone, pick your home Wi-Fi, type the password. It is saved on the ESP32 — you only do this once. The ESP32 must be on the same network as the Roku.

The onboard LED blinks while the ESP32 is waiting for Wi-Fi or for a remote, and goes solid once a remote is connected.

### 4. Pair the remote (once)

1. Press the **BOOT** button on the ESP32 (starts a pairing scan).
2. Press the red **SYNC** button under the remote's battery cover.

The remote's LED 1 lights when it connects. **Use SYNC, not 1+2**: pairing with 1+2 is a temporary "one time" mode that does not save the ESP32's address in the remote, so the remote can never page it back later. SYNC pairing saves it, which is what makes Power-on reconnect work.

After that, pressing **Power** on the remote reconnects it in about a second.

---

## Button map

| Button | Action |
|---|---|
| D-pad | Roku arrows (hold to repeat: first repeat 450 ms, then every 250 ms) |
| A (tap) | Select / OK (fires on press) |
| B (tap, < 300 ms) | Back (fires on release) |
| B (hold 300 ms) + tilt | Pointer mode (see below) |
| Home (tap) | Home |
| Home (hold 2.5 s) | TV on (Roku PowerOn over CEC) |
| Home (hold 4.5 s) | TV off (Roku PowerOff over CEC) |
| + / − | Volume up / down (hold to repeat) |
| 1 | Launch Netflix |
| 2 | Launch Disney+ |
| 1 + 2 together | Mute |
| Power | Disconnects the remote — and that is the TV power switch: turning the remote off sends TV PowerOff once |
| BOOT (on the ESP32) | No remote connected: starts a pairing scan. Remote connected: prints a status summary to Serial |

---

## Pointer mode

Hold **B** for 300 ms, then point at the TV. This is **position control**, not velocity control: the highlight's position follows where you point, the way a Wii pointer follows your hand. Tilt right and the highlight moves right and stays there; tilt back toward center and it comes back. Release B and the highlight stays exactly where it is — tap A to select it.

How it works:

- **Calibrate on entry** — the moment pointer mode engages, the remote's current pose becomes "center" (wherever the highlight already is), so it works from any starting angle and any grip. Hold the remote flat, the way you would point it at the TV.
- **Tilt maps to grid cells** — each ~0.13 g of tilt (about 9°) is one cell. The bridge sends arrow keys to walk the highlight to the cell you are pointing at, one key per 200 ms, so it glides rather than jumps. It only advances on an HTTP 200 from the Roku, so the highlight on screen and the bridge's idea of it stay in sync.
- **Dead zone** — small tilts (~6°) stay on center, so hand tremor never moves anything. The dead zone is circular, so diagonals work as well as straight tilts.
- **Smoothing** — a One-Euro filter adapts to how fast you are moving: strong smoothing when still (kills jitter), light smoothing when moving fast (kills lag).

Tuning constants live near the top of the sketch (`POINT_CELL_X_G`, `POINT_CELL_Y_G`, `POINT_DEADZONE_G`, `POINT_MAX_CELL`). If a direction feels backwards, flip `TILT_X_SIGN` or `TILT_Y_SIGN` — a one-line fix.

**Honest limits:** the bridge tracks the highlight by counting the keys it sent — there is no Roku endpoint that reports the real highlight position, so if you move the highlight with another remote, the bridge's idea of it drifts until you re-press B to re-center. Edge drift does not self-correct. True absolute pointing needs a Wii sensor bar; this build uses tilt only. The Wiimote has no gyroscope — tilt comes from its 3-axis accelerometer reading gravity, so it senses tilt, not arm movement: point by tilting/rolling the remote, not by moving your arm sideways.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Remote will not connect at all | Press BOOT on the ESP32, then SYNC under the battery cover. Make sure the patched 2.0.3 library is installed, not the stock one. |
| Power-on does not reconnect | Re-pair once with SYNC (not 1+2). If a clone remote still will not reconnect after SYNC, it may not store host addresses at all — then 1+2 held ~7 s is the only way in. |
| Upload fails | Use upload speed **115200**, not 921600. |
| Upload hangs on `Connecting........` | Hold the **BOOT** button on the ESP32 as the upload starts, release when the progress bars appear. |
| No port in Tools → Port | Use a USB data cable, and install the CP210x (Silicon Labs) or CH340 (WCH) driver for your board. |
| Sketch will not compile | Install WiFiManager by tzapu from Library Manager. Check the ESP32 board package is core 3.3.12 and the board is ESP32 Dev Module. |
| `ESP32Wiimote.h: No such file or directory` | Install `Fixed ESP32 Wiimote Package.zip` via Sketch → Include Library → Add .ZIP Library, after deleting any old ESP32Wiimote library. |
| Serial Monitor shows nothing | Set the Serial Monitor baud rate to **115200**. |
| Roku does not respond | Check Control by mobile apps is Enabled, and that `ROKU_IP` matches Settings → Network → About. The ESP32 and Roku must be on the same network. |
| TV does not turn on/off | TV power goes through HDMI-CEC — enable CEC on the TV (e.g. LG SIMPLINK) and on the Roku. |
| Volume buttons do nothing | Same CEC requirement. The bridge prints a CEC verdict to Serial on connect — read it to see whether your TV reports volume control. |
| Pointer feels backwards | Flip `TILT_X_SIGN` or `TILT_Y_SIGN` in the sketch. |
| Pointer drifts at the edges | Re-press B to re-center. The bridge counts sent keys and cannot see the real highlight. |
| All 4 LEDs stay lit forever | Some clone remotes report battery 0% permanently — there is no other battery signal, so the LEDs stay at full. |
| ESP32 crashes on first boot | Wi-Fi setup and Bluetooth together crash the ESP32, so the sketch deliberately starts Wi-Fi first and Bluetooth second. After the first Wi-Fi setup it connects in seconds. |

---

## Project status

- Roku control, TV power on disconnect, pointer mode, app shortcuts, battery LEDs: **working and hardware-verified**
- Power-on reconnect via the 2.0.3 listen-first patch: **built and compile-verified, needs one live Power test** — install the library, SYNC re-pair once, press Power
- Volume over CEC: **depends on the TV** — the bridge reports the verdict to Serial on connect
- Not implemented: rumble and speaker (the library has no API for either), sensor-bar IR pointing, Nunchuk support

---

## What's in this repo

| File | What it is |
|---|---|
| `Roku WiiMote Code.ino` | The bridge sketch — the whole project |
| `README.md` | This file |
| `Fixed ESP32 Wiimote Package.zip` | The patched Wii Remote library — install via Add .ZIP Library, never update from Library Manager |

---

## Credits

- Wii Remote library: [andremmfaria/ESP32Wiimote](https://github.com/andremmfaria/ESP32Wiimote) (stock 2.0.0), patched here for listen-first reconnect and NVS persistence
- Wi-Fi setup: [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager)
- Roku External Control API: <https://developer.roku.com/dev/docs/external-control-api>
- Pointer smoothing: One-Euro filter (Casiez et al.)

