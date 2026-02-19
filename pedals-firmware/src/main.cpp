#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Preferences.h>

// Pins (Adjust as needed for your specific wiring)
// Using GPIO 1, 2, 3 as requested/assumed. Note: Check if these are ADC1 on S3.
// ESP32-S3: ADC1_CH0 is GPIO 1, ADC1_CH1 is GPIO 2, ADC1_CH2 is GPIO 3.
#define THROTTLE_PIN 1
#define BRAKE_PIN 2
#define CLUTCH_PIN 3

// Calibration Defaults
#define DEFAULT_MIN 0
#define DEFAULT_MAX 4095

// HID Report Descriptor: 16-bit X, Y, Z
// Usage Page (Generic Desktop), Usage (Joystick)
uint8_t const desc_hid_report[] = {
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

Adafruit_USBD_HID usb_hid;
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
  // If not exists, will return default
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
    long val = raw;
    if (min_v < max_v) {
        val = constrain(val, min_v, max_v);
        return map(val, min_v, max_v, 0, 65535);
    } else {
        // Inverted (e.g. min=4000, max=100)
        // If raw=3000 (pressed partly), we want it mapped.
        // constrain: if raw > min (4000) -> 4000. if raw < max (100) -> 100.
        // But constrain macro fails for inverted range logic if used blindly.
        if (val > min_v) val = min_v;
        if (val < max_v) val = max_v;
        return map(val, min_v, max_v, 0, 65535);
    }
}

void setup() {
  Serial.begin(115200);

  // Setup Pins
  for(int i=0; i<3; i++) pinMode(pins[i], INPUT);

  loadConfig();

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();

  // Wait for USB to be ready
  // while( !TinyUSBDevice.mounted() ) delay(1);
}

void loop() {
  // Read and Smooth
  for(int i=0; i<3; i++) {
    long sum = 0;
    for(int k=0; k<16; k++) sum += analogRead(pins[i]);
    current_raw[i] = sum / 16;
  }

  if (usb_hid.ready()) {
    HidReport report;
    report.x = processPedal(current_raw[0], configs[0].min_val, configs[0].max_val);
    report.y = processPedal(current_raw[1], configs[1].min_val, configs[1].max_val);
    report.z = processPedal(current_raw[2], configs[2].min_val, configs[2].max_val);

    usb_hid.sendReport(0, &report, sizeof(report));
  }

  // Serial Protocol
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "READ") {
      // Return raw values: "RAW:123,456,789"
      Serial.printf("RAW:%d,%d,%d\n", current_raw[0], current_raw[1], current_raw[2]);
    } else if (cmd == "GET_CONFIG") {
      // Return config: "CONF:min:max,min:max,min:max"
      Serial.printf("CONF:%d:%d,%d:%d,%d:%d\n",
        configs[0].min_val, configs[0].max_val,
        configs[1].min_val, configs[1].max_val,
        configs[2].min_val, configs[2].max_val);
    } else if (cmd.startsWith("SET")) {
      // SET idx min max
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
}
