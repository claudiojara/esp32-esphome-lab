# esp32-esphome-lab

Experiments with **ESP32-C3 SuperMini + ESPHome + Home Assistant**. YAML-declarative
firmware (no C++), OTA updates, native HA API with encryption, mDNS auto-discovery.

What lives here:

- LED control from HA (the hello-world of embedded + HA)
- OLED 1.3" SH1106 displays showing HA entities (covers, weather, system info)
- Multi-page rotating dashboard with physical + virtual buttons
- HLK-LD2410C mmWave presence sensor with distance/energy and reactive UI

## Hardware

| Part | Notes |
|------|-------|
| ESP32-C3 SuperMini | `board: esp32-c3-devkitm-1`, `variant: esp32c3`. LED on GPIO8 active-low. |
| OLED 1.3" I2C (SH1106) | GPIO5 SDA / GPIO6 SCL. Requires `contrast: 100%` — default is black screen. |
| HLK-LD2410C mmWave | UART at 256000 baud, GPIO4 TX (to sensor RX), GPIO3 RX (from sensor TX). VCC to 5V. |
| Tactile button | GPIO7 to GND, internal pullup. |

Home Assistant runs as a VM on Proxmox in this setup — the ESPHome HA addon has
no USB access from the HAOS guest, so first flash is always from the Mac via
`esphome` CLI. After that, OTA works from anywhere on the LAN.

## Quick start

Prerequisites: Python 3.10+, `uv` installed.

```bash
# 1. Clone
git clone https://github.com/claudiojara/esp32-esphome-lab.git
cd esp32-esphome-lab

# 2. Install ESPHome CLI (isolated, fast)
uv tool install esphome

# 3. Copy secrets template and fill with your WiFi / OTA credentials
cp secrets.yaml.example secrets.yaml
$EDITOR secrets.yaml

# 4. Pick a YAML and flash via USB (first time)
ls /dev/cu.usbmodem*                        # find your port
esphome run esp32-test.yaml --device /dev/cu.usbmodem<TAB>

# 5. Adopt in HA: Settings -> Devices & Services -> ESPHome -> paste api.encryption.key

# 6. Subsequent flashes are OTA automatically
esphome run esp32-test.yaml
```

Regenerate your own API encryption key per device (do NOT reuse the ones in
this repo's YAMLs — they are examples):

```bash
python3 -c "import secrets, base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"
```

## Project files

| File | What it does |
|------|--------------|
| `esp32-test.yaml` | HA-controlled LED via GPIO8 active-low. Minimal starting point. |
| `esp32-test-interval.yaml` | Standalone 1s LED blink, no HA api. For isolated hardware test. |
| `esp32-test-oled-cover.yaml` | Single-page OLED showing `cover.curtain` position from HA. |
| `esp32-test-oled-weather.yaml` | OLED with clock + dynamic weather icon fetched from `/config/www/weather/<state>.png`. |
| `esp32-test-oled-pages.yaml` | **Main project.** Multi-page dashboard: curtain, weather, system, avatar, mmwave presence. Auto-rotation + physical/virtual buttons + LD2410C presence overlay on every page + reactive avatar. |
| `mmwavetest.yaml` | Standalone LD2410C presence sensor device, no display. |
| `mmwavetest-debug.yaml` | Raw UART byte dump variant for diagnosing bad wiring (cold joints, shorts, baud mismatch). See hex patterns in the skill. |

## Gotchas worth knowing

These cost hours during the build — documented so the next person skips them.

### C3 SuperMini LED is active-low

`output.gpio.inverted: true` on GPIO8 or HA shows the state backwards. First
thing to set up on this board.

### SH1106 1.3" clones need explicit contrast

The default contrast in ESPHome's SH1106 driver is 0 on AliExpress clones — I2C
scan finds the device, init succeeds, but the screen stays completely black.
Always `contrast: 100%` in the `display:` block.

### 1.3" OLED is SH1106, NOT SSD1306

Two different controllers, different init sequences. `0.96" = SSD1306`,
`1.3" = SH1106`. Wrong driver = garbage pixels.

### GPIO3 and GPIO4 pads are physically adjacent on the SuperMini

When soldering UART wires for the LD2410, extra solder bridges them easily. If
your mmwave sensor shows raw byte patterns like `C0:E0:F0:FC:FE:FF` (progressive
ones — an RC ramp), suspect a short. Multimeter in continuity mode between
GPIO3 and GPIO4: must NOT beep.

### LD2410C VCC wants 5V

Datasheet says 5-12V. 3.3V is unreliable — sensor may boot but stream gibberish.

### First compile is slow

3-5 minutes the first time, under 30s after (cached in `.esphome/`). Be patient.

## Agent skill

The [`skill/`](skill/) folder mirrors the `esp32-homeassistant` Claude agent skill.
It captures the full discovery/scaffolding/flashing workflow, display integration
patterns, mmWave debugging techniques, and byte pattern fingerprints for
diagnosing bad wiring. See [`skill/SKILL.md`](skill/SKILL.md).

## License

MIT. Fork it, break it, learn from it.
