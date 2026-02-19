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

// HID Report Descriptor: 16-bit X, Y, Z
// Usage Page (Generic Desktop), Usage (Joystick)
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

// Report Structure
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

  void _onOutput(uint8_t reportId, const uint8_t* data, uint16_t len) {
    // Not used for input-only device
  }

  bool sendReport(HidReport* report) {
    return HID.SendReport(0, report, sizeof(HidReport));
  }
};

CustomGamepad gamepad;
Preferences prefs;

struct PedalConfig {
  uint16_t min_val;
  uint16_t max_val;
};

PedalConfig configs[3];
int pins[3] = {THROTTLE_PIN, BRAKE_PIN, CLUTCH_PIN};
uint16_t current_raw[3];

void loadConfig() {
  prefs.begin("pedals", true); // Read-only
  configs[0].min_val = prefs.getUShort("p0_min", DEFAULT_MIN);
  configs[0].max_val = prefs.getUShort("p0_max", DEFAULT_MAX);
  configs[1].min_val = prefs.getUShort("p1_min", DEFAULT_MIN);
  configs[1].max_val = prefs.getUShort("p1_max", DEFAULT_MAX);
  configs[2].min_val = prefs.getUShort("p2_min", DEFAULT_MIN);
  configs[2].max_val = prefs.getUShort("p2_max", DEFAULT_MAX);
  prefs.end();
}

void saveConfig() {
  prefs.begin("pedals", false); // Read-write
  prefs.putUShort("p0_min", configs[0].min_val);
  prefs.putUShort("p0_max", configs[0].max_val);
  prefs.putUShort("p1_min", configs[1].min_val);
  prefs.putUShort("p1_max", configs[1].max_val);
  prefs.putUShort("p2_min", configs[2].min_val);
  prefs.putUShort("p2_max", configs[2].max_val);
  prefs.end();
}

uint16_t processPedal(uint16_t raw, uint16_t min_v, uint16_t max_v) {
    if (min_v == max_v) return 0;
    long val = raw;
    if (min_v < max_v) {
        val = constrain(val, min_v, max_v);
        return map(val, min_v, max_v, 0, 65535);
    } else {
        if (val > min_v) val = min_v;
        if (val < max_v) val = max_v;
        return map(val, min_v, max_v, 0, 65535);
    }
}

void setup() {
  Serial.begin(115200);

  for(int i=0; i<3; i++) pinMode(pins[i], INPUT);

  loadConfig();

  gamepad.begin();
  USB.begin();
}

void loop() {
  // Read and Smooth
  for(int i=0; i<3; i++) {
    long sum = 0;
    for(int k=0; k<16; k++) sum += analogRead(pins[i]);
    current_raw[i] = sum / 16;
  }

  HidReport report;
  report.x = processPedal(current_raw[0], configs[0].min_val, configs[0].max_val);
  report.y = processPedal(current_raw[1], configs[1].min_val, configs[1].max_val);
  report.z = processPedal(current_raw[2], configs[2].min_val, configs[2].max_val);

  gamepad.sendReport(&report);

  // Serial Protocol
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "READ") {
      Serial.printf("RAW:%d,%d,%d\n", current_raw[0], current_raw[1], current_raw[2]);
    } else if (cmd == "GET_CONFIG") {
      Serial.printf("CONF:%d:%d,%d:%d,%d:%d\n",
        configs[0].min_val, configs[0].max_val,
        configs[1].min_val, configs[1].max_val,
        configs[2].min_val, configs[2].max_val);
    } else if (cmd.startsWith("SET")) {
      int idx, min_v, max_v;
      if (sscanf(cmd.c_str(), "SET %d %d %d", &idx, &min_v, &max_v) == 3) {
        if (idx >= 0 && idx < 3) {
           configs[idx].min_val = (uint16_t)min_v;
           configs[idx].max_val = (uint16_t)max_v;
           Serial.println("OK");
        }
      }
    } else if (cmd == "SAVE") {
      saveConfig();
      Serial.println("SAVED");
    }
  }

  delay(2); // Small delay to prevent flooding if loop is too fast (though HID sends interval)
}
