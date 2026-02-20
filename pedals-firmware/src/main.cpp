#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>
#include <Preferences.h>

// Pins
#define THROTTLE_PIN 1
#define BRAKE_PIN 2
#define CLUTCH_PIN 3

// Button Pins (Shifter)
// Using GPIO 4-11 for simplicity, adjust as needed.
const int button_pins[8] = {4, 5, 6, 7, 8, 9, 10, 11};

// Calibration Defaults
#define DEFAULT_MIN 0
#define DEFAULT_MAX 4095
#define DEFAULT_DZ 0 // 0%

// HID Report Descriptor: 16-bit X, Y, Z + 8 Buttons
static const uint8_t desc_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)

  // Axes (X, Y, Z)
  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x09, 0x01,        //   Usage (Pointer)
  0xA1, 0x00,        //   Collection (Physical)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x09, 0x32,        //     Usage (Z)
  0x15, 0x00,        //     Logical Minimum (0)
  0x26, 0xFF, 0xFF,  //     Logical Maximum (65535)
  0x75, 0x10,        //     Report Size (16)
  0x95, 0x03,        //     Report Count (3)
  0x81, 0x02,        //     Input (Data, Var, Abs)
  0xC0,              //   End Collection

  // Buttons (1-8)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (1)
  0x29, 0x08,        //   Usage Maximum (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data, Var, Abs)

  // Padding (optional for byte alignment, but 3*16 + 8 = 56 bits = 7 bytes. Perfect.)
  // Actually, HID report size is often byte-aligned.
  // 3 * 16 = 48 bits. + 8 bits = 56 bits = 7 bytes.
  // Wait. 48 + 8 = 56. 56 / 8 = 7. Yes.

  0xC0               // End Collection
};

struct __attribute__((packed)) HidReport {
  uint16_t x;
  uint16_t y;
  uint16_t z;
  uint8_t buttons;
};

USBHID HID;

class CustomGamepad : public USBHIDDevice {
public:
  CustomGamepad(void) {
    static bool initialized = false;
    if(!initialized){
      initialized = true;
      HID.addDevice(this, sizeof(desc_hid_report));
    }
  }

  void begin(void) {
    HID.begin();
  }

  uint16_t _onGetDescriptor(uint8_t* buffer) {
    memcpy(buffer, desc_hid_report, sizeof(desc_hid_report));
    return sizeof(desc_hid_report);
  }

  void _onOutput(uint8_t reportId, const uint8_t* data, uint16_t len) {}

  bool sendReport(HidReport* report) {
    return HID.SendReport(0, report, sizeof(HidReport));
  }
};

CustomGamepad gamepad;
Preferences prefs;

struct PedalConfig {
  uint16_t min_val;
  uint16_t max_val;
  uint8_t dz_start; // %
  uint8_t dz_end;   // %
};

PedalConfig configs[3];
int pins[3] = {THROTTLE_PIN, BRAKE_PIN, CLUTCH_PIN};
uint16_t current_raw[3];
uint8_t current_buttons = 0;

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

void loadConfig() {
  prefs.begin("pedals", true); // Read-only
  for(int i=0; i<3; i++) {
    char key[10];
    sprintf(key, "p%d_min", i);
    configs[i].min_val = prefs.getUShort(key, DEFAULT_MIN);
    sprintf(key, "p%d_max", i);
    configs[i].max_val = prefs.getUShort(key, DEFAULT_MAX);
    sprintf(key, "p%d_ds", i);
    configs[i].dz_start = prefs.getUChar(key, DEFAULT_DZ);
    sprintf(key, "p%d_de", i);
    configs[i].dz_end = prefs.getUChar(key, DEFAULT_DZ);
  }
  prefs.end();
}

void saveConfig() {
  prefs.begin("pedals", false); // Read-write
  for(int i=0; i<3; i++) {
    char key[10];
    sprintf(key, "p%d_min", i);
    prefs.putUShort(key, configs[i].min_val);
    sprintf(key, "p%d_max", i);
    prefs.putUShort(key, configs[i].max_val);
    sprintf(key, "p%d_ds", i);
    prefs.putUChar(key, configs[i].dz_start);
    sprintf(key, "p%d_de", i);
    prefs.putUChar(key, configs[i].dz_end);
  }
  prefs.end();
}

uint16_t processPedal(uint16_t raw, PedalConfig* cfg) {
    long min_v = cfg->min_val;
    long max_v = cfg->max_val;
    long val = raw;

    if (min_v == max_v) return 0;

    long range = max_v - min_v;
    long start_v, end_v;

    start_v = min_v + (range * cfg->dz_start / 100);
    end_v = max_v - (range * cfg->dz_end / 100);

    if (min_v < max_v) { // Normal
        if (start_v >= end_v) return 0;
        val = constrain(val, start_v, end_v);
        return map(val, start_v, end_v, 0, 65535);
    } else { // Inverted
        if (start_v <= end_v) return 0;
        if (val > start_v) val = start_v;
        if (val < end_v) val = end_v;
        return map(val, start_v, end_v, 0, 65535);
    }
}

void setup() {
  Serial.begin(115200);
  inputString.reserve(200);

  for(int i=0; i<3; i++) pinMode(pins[i], INPUT);
  for(int i=0; i<8; i++) pinMode(button_pins[i], INPUT_PULLUP); // Active LOW

  loadConfig();

  gamepad.begin();
  USB.begin();
}

void loop() {
  // Read and Smooth Pedals
  for(int i=0; i<3; i++) {
    long sum = 0;
    for(int k=0; k<8; k++) sum += analogRead(pins[i]);
    current_raw[i] = sum / 8;
  }

  // Read Buttons
  current_buttons = 0;
  for(int i=0; i<8; i++) {
    if (digitalRead(button_pins[i]) == LOW) { // Pressed
      current_buttons |= (1 << i);
    }
  }

  // Send HID Report
  // USB.isConnected check removed for stability (handled internally or ignored)
  {
      HidReport report;
      report.x = processPedal(current_raw[0], &configs[0]);
      report.y = processPedal(current_raw[1], &configs[1]);
      report.z = processPedal(current_raw[2], &configs[2]);
      report.buttons = current_buttons;

      gamepad.sendReport(&report);
  }

  // Serial Event (Non-blocking)
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }

  if (stringComplete) {
    inputString.trim();
    if (inputString == "READ") {
      // Return: RAW:x,y,z,buttons_byte
      Serial.printf("RAW:%d,%d,%d,%d\n", current_raw[0], current_raw[1], current_raw[2], current_buttons);
    } else if (inputString == "GET_CONFIG") {
      Serial.printf("CONF:%d:%d:%d:%d,%d:%d:%d:%d,%d:%d:%d:%d\n",
        configs[0].min_val, configs[0].max_val, configs[0].dz_start, configs[0].dz_end,
        configs[1].min_val, configs[1].max_val, configs[1].dz_start, configs[1].dz_end,
        configs[2].min_val, configs[2].max_val, configs[2].dz_start, configs[2].dz_end);
    } else if (inputString.startsWith("SET")) {
      int idx, min_v, max_v, dzs, dze;
      if (sscanf(inputString.c_str(), "SET %d %d %d %d %d", &idx, &min_v, &max_v, &dzs, &dze) == 5) {
        if (idx >= 0 && idx < 3) {
           configs[idx].min_val = (uint16_t)min_v;
           configs[idx].max_val = (uint16_t)max_v;
           configs[idx].dz_start = (uint8_t)dzs;
           configs[idx].dz_end = (uint8_t)dze;
           Serial.println("OK");
        }
      }
    } else if (inputString == "SAVE") {
      saveConfig();
      Serial.println("SAVED");
    }
    inputString = "";
    stringComplete = false;
  }

  delay(10);
}
