---
name: esp32-homeassistant
description: >
  Set up ESP32 microcontrollers and integrate them with Home Assistant via ESPHome —
  YAML-declarative firmware with native HA API, auto-discovery, and OTA updates.
  Trigger: User mentions ESP32, ESP8266, ESPHome, Home Assistant device integration,
  flashing a microcontroller, connecting a sensor/switch/light to HA, or asks to
  build an IoT device that talks to Home Assistant.
license: Apache-2.0
metadata:
  author: gentleman-programming
  version: "1.2"
---

## When to Use

- User wants to connect any ESP32/ESP8266 board to Home Assistant
- User wants to flash firmware to expose sensors, lights, switches, buttons, relays to HA
- User mentions ESPHome, HAOS, or integrating a microcontroller into a home automation setup
- User asks "ESPHome vs PlatformIO" or similar framework comparison for HA integration

## Critical Patterns

### ESPHome is the default — always

For HA integration, ESPHome beats Arduino/PlatformIO/ESP-IDF by an order of magnitude in DX: YAML declarative, native HA API with encryption, auto-discovery via mDNS, built-in OTA, web dashboard — all free. Only suggest PlatformIO/Arduino/ESP-IDF if the user has a specific need ESPHome can't cover (exotic protocols, ultra-precise timings, learning C++ embedded).

### Ask BEFORE scaffolding — never guess the board

The built-in LED GPIO, the USB-serial chip, and the bootloader procedure ALL depend on the exact board. Writing YAML without confirming the board = broken firmware. Always run the discovery phase first.

### Local CLI over HA addon for flashing

The ESPHome addon runs inside HAOS (often on Proxmox/remote host) and has NO access to the developer's local USB. For first flash, use the local `esphome` CLI. After first flash, OTA works from anywhere on the same network.

### Security rule — never put real creds in `*.example` files

`secrets.yaml` is gitignored, `secrets.yaml.example` is NOT. Users frequently edit the `.example` file by mistake. If you see real WiFi/OTA credentials in `secrets.yaml.example`, stop and warn the user immediately.

---

## Discovery Phase (MANDATORY — run before touching any file)

Ask the user these questions and WAIT for answers. Do NOT proceed with scaffolding until you have at least the board and WiFi creds.

| # | Question | Why it matters |
|---|----------|----------------|
| 1 | **Exact board model?** (ESP32 DevKit classic, ESP32-S3 DevKitC-1, ESP32-S3 SuperMini, ESP32-C3 DevKitM-1, ESP32-C3 SuperMini, other) | Determines `board:` + `variant:` + LED GPIO + LED type (binary vs RGB) + inversion |
| 2 | **USB serial port name?** (`ls /dev/cu.*` on macOS, `ls /dev/ttyUSB* /dev/ttyACM*` on Linux) | Used for first flash. Port naming hints at chip type |
| 3 | **Home Assistant setup?** (HAOS, Container, Supervised, Core — and where it runs: Proxmox, Pi, NUC) | Confirms that addon-based flashing won't work → reinforces local CLI approach |
| 4 | **WiFi SSID and password?** | Required for `secrets.yaml`. If user prefers not to share, tell them to fill in themselves after scaffolding |
| 5 | **What should the ESP32 DO?** (blink LED test, DHT22 temp/humidity, PIR motion, relay, button, display, multiple things) | Drives the YAML components beyond WiFi + API + LED |

### USB port name heuristics

| Port name pattern | Likely chip | Board family |
|-------------------|-------------|--------------|
| `USB JTAG/serial debug unit` / `cu.usbmodem*` | Native USB-JTAG (built into SoC) | ESP32-S3 or ESP32-C3 |
| `SLAB_USBtoUART` / `cu.SLAB_*` | Silicon Labs CP210x | Classic ESP32, S2, some S3 boards |
| `wchusbserial*` / `cu.wchusbserial*` | WCH CH340/CH341 | Cheap classic ESP32, NodeMCU-style |
| `cu.usbserial-*` | FTDI FT232 | Older dev kits |

If user says "USB JTAG/serial debug unit" and is unsure of the board → strongly bias toward ESP32-S3 or C3. Ask them to read the chip label on the board.

---

## Board → Built-in LED Reference

This table is load-bearing. Get this wrong and the LED won't blink.

| Board | `board:` value | `variant:` | LED GPIO | LED type | `inverted:` | Has BOOT button? |
|-------|---------------|-----------|----------|----------|-------------|-------------------|
| ESP32 DevKit (classic) | `esp32dev` | `esp32` | GPIO2 | binary | `false` | Yes |
| ESP32-S3 DevKitC-1 | `esp32-s3-devkitc-1` | `esp32s3` | GPIO48 | RGB WS2812 | n/a | Yes |
| ESP32-S3 SuperMini | `esp32-s3-devkitc-1` | `esp32s3` | GPIO48 | RGB WS2812 | n/a | Usually no |
| ESP32-C3 DevKitM-1 | `esp32-c3-devkitm-1` | `esp32c3` | GPIO8 | binary | **`true`** | Yes |
| ESP32-C3 SuperMini | `esp32-c3-devkitm-1` | `esp32c3` | GPIO8 | binary | **`true`** | Usually no |
| ESP8266 NodeMCU | `nodemcuv2` | n/a (use `esp8266:` block) | GPIO2 | binary | **`true`** | Yes |

### Why `inverted: true` for C3

On C3 SuperMini the LED is wired **active-low** — GPIO LOW = LED on, HIGH = off. Without `inverted: true`, "turn on" from HA actually turns it OFF. Always explain this to the user — it's the #1 "why isn't my LED working" gotcha.

### Why RGB LED on S3 needs different config

S3 built-in LED is a WS2812 addressable RGB, NOT a simple GPIO LED. Do NOT use `output: gpio` + `light: binary` — it won't work. Use `light: esp32_rmt_led_strip` with `chipset: WS2812` and `num_leds: 1`.

---

## Tooling Installation

Preferred (macOS/Linux with modern Python):

```bash
uv tool install esphome
esphome version   # verify — should print "Version: 2026.X.X" or later
```

Fallbacks:

```bash
pipx install esphome        # if no uv
brew install esphome        # Homebrew formula exists but lags behind pip
docker pull esphome/esphome # for fully isolated setups
```

**Do NOT** use `pip install esphome` globally — pollutes system Python. If the user insists, use a venv.

---

## Project Scaffolding

Create these files in the project directory:

```
<project>/
├── <device-name>.yaml       # Main ESPHome config
├── secrets.yaml             # Real credentials (GITIGNORED)
├── secrets.yaml.example     # Template with placeholders (committed)
└── .gitignore               # Must include secrets.yaml and .esphome/
```

### `.gitignore` (mandatory)

```
secrets.yaml
.esphome/
```

### `secrets.yaml.example` (template with placeholders ONLY)

```yaml
wifi_ssid: "YOUR_SSID_HERE"
wifi_password: "YOUR_WIFI_PASSWORD"
fallback_password: "fallback1234"
ota_password: "generate-something-long-and-random"
```

### Generate a fresh API encryption key per device

```bash
python3 -c "import secrets, base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"
```

Paste into `api.encryption.key` in the YAML. NEVER reuse the key across devices.

---

## YAML Templates

See [assets/](assets/) for complete working templates:

- [assets/example-a-blink-test.yaml](assets/example-a-blink-test.yaml) — LED blink loopback (life test)
- [assets/example-b-ha-controlled.yaml](assets/example-b-ha-controlled.yaml) — HA-controlled LED (real use)
- [assets/example-c3-supermini.yaml](assets/example-c3-supermini.yaml) — C3 SuperMini canonical starting point
- [assets/example-s3-rgb.yaml](assets/example-s3-rgb.yaml) — S3 with RGB WS2812 built-in LED
- [assets/example-display-oled-sh1106.yaml](assets/example-display-oled-sh1106.yaml) — LED + OLED 1.3" SH1106 I²C showing a HA cover entity's `current_position` attribute. Single-page.
- [assets/example-display-multi-page.yaml](assets/example-display-multi-page.yaml) — Multi-page dashboard (cover, weather w/ dynamic icon, system info, embedded avatar). Switch + button in HA control auto-rotation. Reference for the whole display ecosystem.
- [assets/example-ha-online-image.yaml](assets/example-ha-online-image.yaml) — `online_image` pattern: fetch PNGs from HA's `/config/www/` dynamically based on a text_sensor state (weather icons).

### Canonical top-level structure

```yaml
esphome:
  name: <device-name>
  friendly_name: <Display Name>

esp32:
  board: <from table>
  variant: <from table>
  framework:
    type: arduino

logger:

api:
  encryption:
    key: "<generated-base64-key>"

ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "<Device> Fallback"
    password: !secret fallback_password

captive_portal:
```

---

## Two Recommended Examples

### Example A — Blink test (life signal)

Adds an `interval:` that toggles the LED every 1s from firmware. Confirms the device is alive and running your code even before HA adoption.

```yaml
output:
  - platform: gpio
    pin:
      number: GPIO8       # C3 SuperMini; adjust per board
      inverted: true      # Required for C3
    id: led_builtin

light:
  - platform: binary
    name: "LED Built-in"
    output: led_builtin
    id: led

interval:
  - interval: 1s
    then:
      - light.toggle: led
```

**Warning to the user**: the `interval` will fight HA control — every second the firmware will toggle the LED regardless of HA state. This is FINE for the initial "is it alive?" test, but MUST be removed before trying to actually control the LED from HA.

### Example B — HA-controlled (production)

Same as A but without the `interval:` block. HA has exclusive control via the `light.led_built_in` entity.

---

## Display Integration (OLED / TFT)

Displays are the #2 request after LEDs. This section captures the hardware selection, wiring, and the gotchas that burn hours of debug time.

### Display selection cheatsheet

| Display | Tech | Res | Color | Bus | Price | Use for |
|---------|------|-----|-------|-----|-------|---------|
| SSD1306 0.96" | OLED | 128×64 | 1-bit | I²C | ~$3 | Text-only micro-status |
| **SH1106 1.3"** | OLED | 128×64 | 1-bit | I²C | ~$3-5 | Text + simple icons (default) |
| **ST7789 1.3"** | IPS LCD | 240×240 | 16-bit color | SPI | ~$5-8 | Dashboards, icons, photos |
| GC9A01 1.28" round | IPS LCD | 240×240 | 16-bit color | SPI | ~$6-10 | Watch-like circular UIs |
| ILI9341 2.4/2.8" | TFT | 320×240 | 16-bit | SPI | ~$8-15 | Wall panels, bigger dashboards |
| E-paper (Waveshare 2.9") | E-ink | varies | 1 or 3 color | SPI | ~$20+ | Low-power always-on (slow refresh) |

**Do NOT recommend e-paper** for anything with a moving clock or frequent updates — refresh takes 1–3s and has ghosting.

### 1.3" OLED is SH1106, NOT SSD1306

The most common bite. Rule of thumb:
- **0.96" OLED → SSD1306**
- **1.3" OLED → SH1106** (different controller, different init sequence)

Wrong driver → display either blank or shows RAM garbage (chip ACKs but init commands are wrong). Use `model: "SH1106 128x64"` on 1.3" boards.

### SH1106 clone gotcha: `contrast: 100%` is mandatory

On most AliExpress SH1106 1.3" clones, ESPHome's default contrast renders the display **completely black** even though I²C scan finds the device and logs show the display initialized fine. Always set `contrast: 100%` explicitly:

```yaml
display:
  - platform: ssd1306_i2c
    model: "SH1106 128x64"
    address: 0x3C
    contrast: 100%   # ← REQUIRED on SH1106 clones
```

Symptom → fix triage:

| Symptom | Cause | Fix |
|---------|-------|-----|
| I²C scan finds 0x3C, but display is black | SH1106 default contrast is 0 | Add `contrast: 100%` |
| Everything shows shifted 2px right with garbage edge | Wrong driver model (SSD1306 init on SH1106) | Set `model: "SH1106 128x64"` |
| Display full of random pixels | Hardware works but init mismatched | Power cycle (unplug USB 5s) — OTA doesn't reset OLED chip |
| Nothing found on I²C scan | Wiring / power / wrong pins | Check VDD=3.3V not 5V, SDA/SCL not swapped |
| Pixels show but dim / faded | Low contrast | `contrast: 100%` |

**Always power cycle after flashing a display-adjacent firmware change.** OTA resets the MCU but not the peripherals — the OLED can hang in a weird state from the previous firmware's init.

### I²C pinout for ESP32 common boards

Pick non-strapping, non-USB pins:

| Board | Recommended SDA / SCL | Avoid (strapping / USB / serial) |
|-------|-----------------------|-----------------------------------|
| ESP32-C3 SuperMini | GPIO5 / GPIO6 | GPIO2, 8 (LED), 9, 18/19 (USB), 20/21 (UART) |
| ESP32-S3 DevKitC-1 | GPIO8 / GPIO9 | GPIO0, 45, 46 (strapping), 19/20 (USB) |
| ESP32 DevKit (classic) | GPIO21 / GPIO22 (defaults) | GPIO0, 2, 5, 12, 15 (strapping) |

I²C pins are **reassignable by software** on all ESP32 variants — the "default" pins are conventions, not hardware requirements. Pick any free pin.

### HA data → Display patterns

Four common patterns for showing HA data on a display:

**1. Entity state (number) → `homeassistant` sensor**

```yaml
sensor:
  - platform: homeassistant
    id: living_temp
    entity_id: sensor.living_temperature
```

**2. Entity attribute (number) → `homeassistant` sensor with `attribute:`**

Covers, weather, media_player etc. expose data in attributes, not state. E.g. cover position is `current_position` attribute (not state = "open"/"closed"):

```yaml
sensor:
  - platform: homeassistant
    id: curtain_position
    entity_id: cover.curtain
    attribute: current_position
```

**3. Entity state (string) → `homeassistant` text_sensor**

For weather states ("sunny", "rainy"), locks, climate modes, etc. Use `on_value` to trigger reactive actions:

```yaml
text_sensor:
  - platform: homeassistant
    id: weather_state
    entity_id: weather.home
    on_value:
      - lambda: |-
          // do something with id(weather_state).state
```

**4. Dynamic image from HA → `online_image` + state change**

Fetch PNGs hosted in HA's `/config/www/` folder. Served at `http://<HA-IP>:8123/local/<path>.png` with no authentication. Requires `http_request` as sibling component.

See [assets/example-ha-online-image.yaml](assets/example-ha-online-image.yaml) for the full pattern.

### Multi-page dashboards

Use `display: pages:` (list of lambdas) + an `interval:` with `switch:` template to let HA toggle auto-rotation, plus a `button:` template to advance manually. See [assets/example-display-multi-page.yaml](assets/example-display-multi-page.yaml).

Key idiom: after `display.page.show_next` call `component.update: <display_id>` to force an immediate redraw — without it you wait one full `update_interval` to see the change.

### Fonts

Skip local TTF files. Use Google Fonts directly:

```yaml
font:
  - file: "gfonts://Roboto"
    id: font_small
    size: 12
  - file: "gfonts://Roboto@700"   # 700 = bold weight
    id: font_big
    size: 32
```

ESPHome downloads and embeds the font at compile time. First compile is slow; subsequent are cached.

### Embedded images (compile-time)

For static images (logos, avatars), use the `image:` component with dithering:

```yaml
image:
  - file: "avatar.png"
    id: img_avatar
    resize: 64x64
    type: BINARY
    dither: FloydSteinberg   # for photos/gradients
```

For 1-bit displays, `FloydSteinberg` is essential — simple thresholding destroys detail. Keep expectations low: a full-color photo → 1-bit is lossy. For monochrome displays, prefer simple silhouette icons to photos.

---

## Radar / mmWave Sensors

mmWave presence sensors (LD2410 family especially) are a big step up from PIR: detect **stillness** (not just motion), give you distance and energy per frame, and cost ~$5-10. ESPHome has a first-class `ld2410` component.

### Module selection

| Module | Tech | UART baud | Protocol | Price | Notes |
|--------|------|-----------|----------|-------|-------|
| **HLK-LD2410C** | 24GHz mmWave | 256000 | Binary | ~$5-8 | Default recommendation. Two zones (moving + still). |
| HLK-LD2410B | 24GHz | 256000 | Binary | ~$5 | Older, same protocol as LD2410C |
| HLK-LD2420 | 24GHz | 256000 | Different protocol | ~$4 | NOT compatible with `ld2410` component |
| HLK-LD2450 | 24GHz | 256000 | Different | ~$8-10 | Tracks 2D position (X, Y). Use `ld2450` component |
| HLK-LD1125H / LD1115H | 24GHz (old) | **9600** | ASCII text | ~$4 | Different component, ASCII output |
| RCWL-0516 | Doppler | none | GPIO pulse | ~$1 | No UART, binary motion only. Use `binary_sensor: gpio` |

**If you see `HLK-LD24XX`**: confirm it's 2410 specifically before configuring. Clones often mislabel. 2420/2450/2410 are NOT interchangeable.

### LD2410C wiring

Sensor has 5 pads: `TX RX OUT GND VCC`. ESPHome uses UART, so `OUT` pin (digital presence output) is **not needed** — leave it unconnected.

- `VCC` → **5V** (datasheet 5-12V, 3.3V is unreliable)
- `GND` → GND
- `TX` (sensor) → **ESP RX pin**
- `RX` (sensor) → **ESP TX pin**
- `OUT` → **unconnected**

### Canonical UART + ld2410 config

```yaml
uart:
  id: ld2410_uart
  tx_pin: GPIO4       # ESP transmits here → sensor RX
  rx_pin: GPIO3       # ESP receives here ← sensor TX
  baud_rate: 256000
  parity: NONE
  stop_bits: 1

ld2410:
  uart_id: ld2410_uart

binary_sensor:
  - platform: ld2410
    has_target:          {name: Presence,      id: mmwave_presence}
    has_moving_target:   {name: Moving Target, id: mmwave_moving_target}
    has_still_target:    {name: Still Target,  id: mmwave_still_target}

sensor:
  - platform: ld2410
    moving_distance:    {name: Moving Distance,    id: mmwave_moving_distance}
    still_distance:     {name: Still Distance,     id: mmwave_still_distance}
    moving_energy:      {name: Move Energy,        id: mmwave_moving_energy}
    still_energy:       {name: Still Energy,       id: mmwave_still_energy}
    detection_distance: {name: Detection Distance, id: mmwave_detection_distance}
```

Add optional `number:` (calibration from HA) and `button:` (factory reset, restart) platforms — the component supports them.

### GPIO3/GPIO4 short gotcha — ESP32-C3 SuperMini

On the C3 SuperMini, **GPIO3 and GPIO4 pads are physically adjacent on the same edge**. Soldering both at once with generous solder easily creates a short between them — the #1 reason a mmWave sensor "doesn't work" on this board.

**Prevention**: tin each wire separately, use fine solder, apply flux, make conical joints. After soldering, **verify with a multimeter in continuity mode — GPIO3 ↔ GPIO4 must NOT beep**. If it does, clean with solder wick and redo.

### UART debug: raw byte dump (diagnostic tool)

When the `ld2410` parser warns `Max command length exceeded; ignoring` nonstop, drop the component entirely and dump raw bytes to identify the actual problem:

```yaml
# Use TEMPORARILY — replaces the ld2410 block
uart:
  id: ld2410_uart
  tx_pin: GPIO4
  rx_pin: GPIO3
  baud_rate: 256000
  parity: NONE
  stop_bits: 1
  debug:
    direction: RX
    dummy_receiver: true
    after:
      bytes: 32
    sequence:
      - lambda: UARTDebug::log_hex(direction, bytes, ':');
```

(Remove the `ld2410:` block while using this — they can't share the same UART with `dummy_receiver: true`.)

### Byte pattern fingerprints (interpreting raw dumps)

Reading hex dumps is the forensics of embedded debugging. These patterns are load-bearing:

| Pattern | Diagnosis | Example |
|---------|-----------|---------|
| `F4:F3:F2:F1 ... F8:F7:F6:F5` | Valid LD2410 data frame | Working! Parser bug if still warning |
| `FD:FC:FB:FA ... 04:03:02:01` | Valid LD2410 command ACK | Working |
| **`C0:E0:F0:FC:FE:FF` (progressive ones)** | **RC ramp — SHORT circuit or high capacitance on line** | Check GPIO3↔GPIO4 solder bridge |
| `00:00:00:00:00:00 00:FF:00:00` (mostly zeros, sparse noise) | Line idle — sensor not transmitting | No power, cold TX joint, or dead sensor |
| Continuous random bytes, no structure | Baud rate mismatch | Try alternate bauds (9600, 115200, 256000) |
| All `0xFF` | Line pulled up, no driver | Sensor not powered OR TX floating |

The progressive-ones pattern is non-obvious but definitive: a digital line shouldn't take 11 bytes to transition from low to high. If it does, the edge is being slowed by RC behavior — either a short to an adjacent driven line, or high cable capacitance + weak drive.

### Combining mmWave with a display

Three patterns worth knowing for display + presence integration:

**1. Presence icon overlay on every page** — tiny filled circle top-center, visible only when detected:

```cpp
// At the end of every page lambda:
if (id(mmwave_presence).state) {
  it.filled_circle(62, 4, 2);
}
```

**2. Reactive text** — avatar/greeting that changes based on presence:

```cpp
if (id(mmwave_presence).state) {
  it.printf(96, 8, id(font_small), TextAlign::TOP_CENTER, "TE VEO");
} else {
  it.printf(96, 8, id(font_small), TextAlign::TOP_CENTER, "HOLA");
}
```

**3. Dedicated page** — show moving vs still target with distance and energy:

```cpp
if (id(mmwave_presence).state) {
  const char* tipo = id(mmwave_moving_target).state ? "MOVIL" : "QUIETO";
  it.printf(64, 18, id(font_medium), TextAlign::TOP_CENTER, "%s", tipo);
  float dist = id(mmwave_moving_target).state
    ? id(mmwave_moving_distance).state
    : id(mmwave_still_distance).state;
  it.printf(0, 40, id(font_small), "Dist: %.0f cm", dist);
} else {
  it.printf(64, 28, id(font_medium), TextAlign::TOP_CENTER, "VACIO");
}
```

### LD2410 calibration tips for the user

- **Moving energy > Still energy** is normal; people generate stronger radar returns when moving
- Default `timeout` is 5s — presence lingers that long after the person leaves the zone. Tune from HA via the exposed `number:` entities
- **Detection gates**: the sensor divides range into 9 gates of 0.75m each (up to ~6m). `max_move_distance_gate` and `max_still_distance_gate` let you ignore far-away detections
- Mount the sensor **away from metal** and **not behind plastic that contains metallic paint/coating** — radar passes through plastic and glass but not foil-backed materials

---

## Flashing Workflow

```bash
esphome run <device-name>.yaml
```

The command runs three phases in sequence:

1. **Compile** — first run downloads the Espressif toolchain + framework. Takes **3–5 minutes** the first time, under 30s on subsequent runs (cached in `.esphome/`).
2. **Upload** — prompts for the method. Options:
   - USB serial port (first time) — select `/dev/cu.usbmodem*` or equivalent
   - OTA (after first flash) — ESPHome auto-detects via mDNS
3. **Monitor** — opens live log stream. Exit with `Ctrl+C`.

### C3 SuperMini bootloader gotcha

Many C3 SuperMini clones have **no physical BOOT button**. If `esphome run` fails with `Failed to connect to ESP32-C3: No serial data received.`, force bootloader mode manually:

1. Disconnect USB
2. Jumper **GPIO9 to GND** with a wire
3. Connect USB (board enters download mode)
4. Run `esphome run <device>.yaml`
5. After flash completes, remove the jumper and press RESET (or re-plug USB)

If the board HAS a BOOT button: hold BOOT → press RESET → release BOOT → flash.

---

## Home Assistant Adoption

### Auto-discovery (preferred)

1. Open HA → **Settings → Devices & Services**
2. In **Discovered** section look for `ESPHome: <device-name>`
3. Click **CONFIGURE**
4. Paste the **encryption key** from the YAML (`api.encryption.key`)
5. Submit → device appears with all its entities

### Manual fallback (if not discovered)

1. **Settings → Devices & Services → + ADD INTEGRATION**
2. Search "ESPHome"
3. Host: `<device-name>.local` (mDNS) or the IP from serial logs
4. Port: `6053` (do not change)
5. Encryption key: from YAML

### Verification

From the Mac:
```bash
ping <device-name>.local    # Should resolve via mDNS
```

If mDNS fails, use the IP directly (visible in `esphome logs` output).

---

## Iteration After First Flash

Once the device is on WiFi, **all subsequent flashes are OTA automatically**:

```bash
esphome run <device-name>.yaml
# → ESPHome auto-detects via mDNS, uses OTA, no cable needed
```

No more bootloader dances, no more USB port selection.

---

## Commands Reference

```bash
esphome run <file>          # Compile + upload + monitor (the main command)
esphome compile <file>      # Just compile, no upload
esphome upload <file>       # Just upload (skip compile)
esphome logs <file>         # Live log viewer (serial or network)
esphome config <file>       # Validate YAML without compiling
esphome clean <file>        # Clear build cache (use if weird errors)
esphome wizard <file>       # Interactive wizard (rarely useful)
esphome version             # Version check
```

---

## Common Errors and Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `Error reading file secrets.yaml: [Errno 2]` | User only has `secrets.yaml.example`, not `secrets.yaml` | `cp secrets.yaml.example secrets.yaml` and fill with real creds |
| `Failed to connect to ESP32-C3: No serial data received` | Board not in bootloader mode | Force bootloader: GPIO9 → GND, replug USB |
| Device not in HA "Discovered" | mDNS blocked, wrong subnet, or device not on WiFi | `ping <name>.local`, check `esphome logs` for WiFi connect |
| LED is on when HA says "off" (and vice versa) | Missing `inverted: true` on C3/C3-SuperMini GPIO | Add `inverted: true` to the `output:` pin block |
| Encryption key mismatch on adoption | Key in HA ≠ key in YAML | Copy key from YAML `api.encryption.key` exactly |
| Compile works but upload hangs | Wrong USB port selected | `ls /dev/cu.*` — pick the right one |
| Real WiFi password in `secrets.yaml.example` | User edited the wrong file | Revert `.example` to placeholders, rotate WiFi password if committed |
| `Component online_image requires component http_request` | Missing sibling component | Add top-level `http_request:` block |
| OLED 1.3" found on I²C scan but screen stays black | SH1106 clone with default contrast=0 | Add `contrast: 100%` to `display:` block |
| OLED shows garbled/random pixels | Wrong driver model (likely SSD1306 configured for SH1106 chip) | Change `model:` to `"SH1106 128x64"` |
| HA entity state never arrives (no `Got state` log line) | Wrong `entity_id` (entity doesn't exist in HA) | Check exact ID in HA → Developer Tools → States |

---

## Security Checklist

Before declaring "done":

- [ ] `secrets.yaml` is in `.gitignore`
- [ ] `secrets.yaml.example` contains ONLY placeholder values (no real SSIDs/passwords)
- [ ] API encryption key is fresh per device (not copy-pasted across devices)
- [ ] OTA password is long and random (not `1234` or `admin`)
- [ ] `.esphome/` build cache is in `.gitignore`
- [ ] If any real credential was ever committed to git, warn the user to rotate it

---

## Teaching Style (how to talk to the user)

- **Ask first, scaffold second.** Run the discovery phase before writing any YAML.
- **Explain WHY**, especially for:
  - `inverted: true` on the C3 (wired active-low)
  - Board-specific GPIO pins (not all ESP32s are the same)
  - Why we remove the blink interval for real HA control (firmware fights HA)
  - Why a fresh encryption key per device (compromise blast radius)
- **Match the user's language.** If the user writes in Spanish, explain in Spanish (Rioplatense is welcome). Technical terms (`GPIO`, `OTA`, `mDNS`, `YAML`) stay in English.
- **Warn proactively** about security slips (real creds in `.example`, weak OTA passwords, reusing encryption keys).

---

## Resources

- **Templates**: See [assets/](assets/) for four ready-to-use YAML examples
- **ESPHome docs**: https://esphome.io/
- **HA ESPHome integration**: https://www.home-assistant.io/integrations/esphome/
