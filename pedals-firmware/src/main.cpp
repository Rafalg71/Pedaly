#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>
#include <Preferences.h>

// Pins
#define THROTTLE_PIN 1
#define BRAKE_PIN 2
#define CLUTCH_PIN 3

// Calibration Defaults
#define DEFAULT_MIN 0
#define DEFAULT_MAX 4095
#define DEFAULT_DZ 0 // 0%

// HID Report Descriptor: 16-bit X, Y, Z
static const uint8_t desc_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)
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
  0xC0               // End Collection
};

struct __attribute__((packed)) HidReport {
  uint16_t x;
  uint16_t y;
  uint16_t z;
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

    // Calculate effective range with deadzones
    long range = max_v - min_v;
    long start_v, end_v;

    // Normal or Inverted Logic handled by map() if we define bounds correctly
    // But deadzones are relative to the "physical travel".
    // Let's normalize to 0-100% first relative to raw min/max, then apply deadzone, then scale.

    // Simpler: Adjust min_v and max_v based on percentage
    // If Normal: min < max. range is positive.
    // If Inverted: min > max. range is negative.

    start_v = min_v + (range * cfg->dz_start / 100);
    end_v = max_v - (range * cfg->dz_end / 100);

    // Apply constraints based on new start/end
    if (min_v < max_v) { // Normal
        if (start_v >= end_v) return 0; // Config Error or overlap
        val = constrain(val, start_v, end_v);
        return map(val, start_v, end_v, 0, 65535);
    } else { // Inverted
        // e.g. min=4000, max=0. range=-4000.
        // dz_start=10%. start = 4000 + (-400) = 3600.
        // dz_end=10%. end = 0 - (-400) = 400.
        // map(val, 3600, 400, 0, 65535).
        // if val=3800 (released), it's > start. constrain to start.
        if (start_v <= end_v) return 0; // Config Error

        // Custom constrain for inverted range
        if (val > start_v) val = start_v;
        if (val < end_v) val = end_v;

        return map(val, start_v, end_v, 0, 65535);
    }
}

void setup() {
  Serial.begin(115200);
  inputString.reserve(200);

  for(int i=0; i<3; i++) pinMode(pins[i], INPUT);

  loadConfig();

  gamepad.begin();
  USB.begin();
}

void loop() {
  // Read and Smooth
  for(int i=0; i<3; i++) {
    long sum = 0;
    // Reduce samples to 8 for speed
    for(int k=0; k<8; k++) sum += analogRead(pins[i]);
    current_raw[i] = sum / 8;
  }

  // USB.isConnected() might not be available in all core versions or specific modes.
  // TinyUSB usually handles this internally, but for built-in USBHID, we can check if it's mounted?
  // Actually, sending report usually returns false if not connected.
  // Let's just try sending.
  {
      HidReport report;
      report.x = processPedal(current_raw[0], &configs[0]);
      report.y = processPedal(current_raw[1], &configs[1]);
      report.z = processPedal(current_raw[2], &configs[2]);

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
      Serial.printf("RAW:%d,%d,%d\n", current_raw[0], current_raw[1], current_raw[2]);
    } else if (inputString == "GET_CONFIG") {
      // CONF:min:max:dzs:dze,...
      Serial.printf("CONF:%d:%d:%d:%d,%d:%d:%d:%d,%d:%d:%d:%d\n",
        configs[0].min_val, configs[0].max_val, configs[0].dz_start, configs[0].dz_end,
        configs[1].min_val, configs[1].max_val, configs[1].dz_start, configs[1].dz_end,
        configs[2].min_val, configs[2].max_val, configs[2].dz_start, configs[2].dz_end);
    } else if (inputString.startsWith("SET")) {
      // SET idx min max dzs dze
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

  delay(10); // Relieve CPU
}
