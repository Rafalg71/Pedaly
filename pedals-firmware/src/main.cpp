#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>

// Button Pins (Shifter)
// Using GPIO 4-11 for simplicity, adjust as needed.
const int button_pins[8] = {4, 5, 6, 7, 8, 9, 10, 11};

// HID Report Descriptor: 8 Buttons
static const uint8_t desc_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)

  // Buttons (1-8)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (1)
  0x29, 0x08,        //   Usage Maximum (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data, Var, Abs)

  0xC0               // End Collection
};

struct __attribute__((packed)) HidReport {
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

uint8_t current_buttons = 0;

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

void setup() {
  Serial.begin(115200);
  inputString.reserve(200);

  for(int i=0; i<8; i++) pinMode(button_pins[i], INPUT_PULLUP); // Active LOW

  gamepad.begin();
  USB.begin();
}

void loop() {
  // Read Buttons
  current_buttons = 0;
  for(int i=0; i<8; i++) {
    if (digitalRead(button_pins[i]) == LOW) { // Pressed
      current_buttons |= (1 << i);
    }
  }

  // Send HID Report
  {
      HidReport report;
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
      // Return: BTN:buttons_byte
      Serial.printf("BTN:%d\n", current_buttons);
    }
    inputString = "";
    stringComplete = false;
  }

  delay(10);
}
