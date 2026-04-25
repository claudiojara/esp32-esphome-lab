// hourglass - Phase 1.5: blink builtin LED to confirm PlatformIO toolchain
// works on the ESP32-C3 SuperMini.
//
// Builtin LED is on GPIO8, active-low (LED on = pin LOW).
// Once this works, we move to Phase 2.A (chain MAX7219 + draw static pattern).

#include <Arduino.h>

constexpr uint8_t LED_PIN = 8;  // ESP32-C3 SuperMini builtin LED (active-low)

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // start OFF (active-low)

  delay(1000);
  Serial.println();
  Serial.println("=== hourglass Phase 1.5 ===");
  Serial.println("If you see 'blink' messages and the LED toggles every 500ms,");
  Serial.println("PlatformIO + USB CDC + GPIO output are all working.");
}

void loop() {
  digitalWrite(LED_PIN, LOW);   // ON
  delay(500);
  digitalWrite(LED_PIN, HIGH);  // OFF
  delay(500);
  Serial.println("blink");
}
